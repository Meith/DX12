#include "gpu_interface.h"
#include "error.h"
#include "misc.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "d3dcompiler.lib")


void create_gpu_device(struct gpu_device_info *device_info)
{
        HRESULT result;

        // Enable debug layer
        #if defined(_DEBUG)
        result = D3D12GetDebugInterface(&IID_ID3D12Debug,
                &device_info->debug);
        show_error_if_failed(result);

        ID3D12Debug_EnableDebugLayer(device_info->debug);
        #endif

        // Create a DXGI factory
        result = CreateDXGIFactory2(0, &IID_IDXGIFactory5,
                &device_info->factory5);
        show_error_if_failed(result);

        IDXGIAdapter1 *adapter1;
        for (UINT i = 0;
                DXGI_ERROR_NOT_FOUND !=
                IDXGIFactory5_EnumAdapters1(device_info->factory5,
                        i, &adapter1);
                ++i)
        {
                DXGI_ADAPTER_DESC1 desc1;
                result = IDXGIAdapter1_GetDesc1(adapter1, &desc1);
                show_error_if_failed(result);

                if (desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                        // Don't select the Basic Render Driver adapter.
                        continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the
                // actual device yet.
                if (SUCCEEDED(D3D12CreateDevice((IUnknown *) adapter1,
                        D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, NULL)))
                {
                    break;
                }

                adapter1 = NULL;
        }

        assert(adapter1 != NULL);
        IUnknown *final_adapter = (IUnknown *)adapter1;
        result = D3D12CreateDevice(final_adapter, D3D_FEATURE_LEVEL_11_0,
                &IID_ID3D12Device, &device_info->device);
        show_error_if_failed(result);

        result = ID3D12Object_SetName(device_info->device, L"Device");
        show_error_if_failed(result);
}

void release_gpu_device(struct gpu_device_info *device_info)
{
        ID3D12Device_Release(device_info->device);

        // Release DXGI factory
        IDXGIFactory5_Release(device_info->factory5);

        #if defined(_DEBUG)
        ID3D12Debug_Release(device_info->debug);
        #endif
}


void create_cmd_queue(struct gpu_device_info *device_info,
        struct gpu_cmd_queue_info *cmd_queue_info)
{
        D3D12_COMMAND_QUEUE_DESC desc;
        desc.Type = cmd_queue_info->type;
        desc.Priority = 0;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        HRESULT result;

        result = ID3D12Device_CreateCommandQueue(device_info->device, &desc,
                &IID_ID3D12CommandQueue, &cmd_queue_info->cmd_queue);
        show_error_if_failed(result);

        result = ID3D12Object_SetName(cmd_queue_info->cmd_queue,
                cmd_queue_info->name);
        show_error_if_failed(result);
}

void release_cmd_queue(struct gpu_cmd_queue_info *cmd_queue_info)
{
        ID3D12CommandQueue_Release(cmd_queue_info->cmd_queue);
}


void create_resource(struct gpu_device_info *device_info,
        struct gpu_resource_info *resource_info)
{
        D3D12_HEAP_PROPERTIES heap_properties;
        heap_properties.Type = resource_info->type;
        heap_properties.CPUPageProperty =
                D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_properties.MemoryPoolPreference =
                D3D12_MEMORY_POOL_UNKNOWN;
        heap_properties.CreationNodeMask = 0;
        heap_properties.VisibleNodeMask = 0;

        D3D12_RESOURCE_DESC resource_desc;
        resource_desc.Dimension = resource_info->dimension;
        resource_desc.Alignment = 0;
        resource_desc.Width = resource_info->width;
        resource_desc.Height = resource_info->height;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels = resource_info->mip_levels;
        resource_desc.Format = resource_info->format;
        resource_desc.SampleDesc.Count = 1;
        resource_desc.SampleDesc.Quality = 0;
        resource_desc.Layout = resource_info->layout;
        resource_desc.Flags = resource_info->flags;

        D3D12_CLEAR_VALUE *clear_value_ptr = NULL;
        D3D12_CLEAR_VALUE clear_value;
        clear_value.Format = resource_info->format;
        switch (resource_info->flags)
        {
                case D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL:
                        clear_value.DepthStencil.Depth = 1.0f;
                        clear_value.DepthStencil.Stencil = 0;
                        clear_value_ptr = &clear_value;
                        break;

                case D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET:
                        clear_value.Color[0] = 0.0f;
                        clear_value.Color[1] = 0.0f;
                        clear_value.Color[2] = 0.0f;
                        clear_value.Color[3] = 1.0f;
                        clear_value_ptr = &clear_value;
                        break;

                default:
                     break;
        }

        HRESULT result;

        result = ID3D12Device_CreateCommittedResource(device_info->device,
                &heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc,
                resource_info->current_state, clear_value_ptr,
                &IID_ID3D12Resource, &resource_info->resource);
        show_error_if_failed(result);

        if (resource_info->dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
                resource_info->gpu_address =
                        ID3D12Resource_GetGPUVirtualAddress(resource_info->resource);

        result = ID3D12Object_SetName(resource_info->resource,
                resource_info->name);
        show_error_if_failed(result);
}

void release_resource(struct gpu_resource_info *resource_info)
{
        ID3D12Resource_Release(resource_info->resource);
}

void upload_resources(struct gpu_resource_info *resource_info, void *src_data)
{
        void *dst_data;

        HRESULT result;

        result = ID3D12Resource_Map(resource_info->resource, 0, NULL,
                &dst_data);

        show_error_if_failed(result);

        memcpy(dst_data, src_data, resource_info->width);
        
        ID3D12Resource_Unmap(resource_info->resource, 0, NULL);
}


void create_descriptor(struct gpu_device_info *device_info,
        struct gpu_descriptor_info *descriptor_info)
{
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
        heap_desc.Type = descriptor_info->type;
        heap_desc.NumDescriptors = descriptor_info->num_descriptors;
        heap_desc.Flags = descriptor_info->flags;
        heap_desc.NodeMask = 0;

        HRESULT result;

        result = ID3D12Device_CreateDescriptorHeap(device_info->device,
                &heap_desc, &IID_ID3D12DescriptorHeap,
                &descriptor_info->descriptor_heap);
        show_error_if_failed(result);

        descriptor_info->stride =
                ID3D12Device_GetDescriptorHandleIncrementSize(
                        device_info->device, heap_desc.Type);

        ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(
                descriptor_info->descriptor_heap,
                &descriptor_info->base_cpu_handle);

        descriptor_info->cpu_handle = descriptor_info->base_cpu_handle;
        
        ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(
                descriptor_info->descriptor_heap,
                &descriptor_info->base_gpu_handle);

        descriptor_info->gpu_handle = descriptor_info->base_gpu_handle;

        result = ID3D12Object_SetName(descriptor_info->descriptor_heap,
                descriptor_info->name);
        show_error_if_failed(result);
}

void release_descriptor(struct gpu_descriptor_info *descriptor_info)
{
        ID3D12DescriptorHeap_Release(descriptor_info->descriptor_heap);
}

void update_cpu_handle(struct gpu_descriptor_info *descriptor_info,
        UINT index)
{
        descriptor_info->cpu_handle.ptr = descriptor_info->base_cpu_handle.ptr +
                index * descriptor_info->stride;
}

void update_gpu_handle(struct gpu_descriptor_info *descriptor_info, 
        UINT index)
{
        descriptor_info->gpu_handle.ptr = descriptor_info->base_gpu_handle.ptr +
                index * descriptor_info->stride;
}

void create_rendertarget_view(struct gpu_device_info *device_info,
        struct gpu_descriptor_info *descriptor_info,
        struct gpu_resource_info *resource_info)
{
        for (UINT i = 0; i < descriptor_info->num_descriptors; ++i) {
                D3D12_RENDER_TARGET_VIEW_DESC rtv_desc;
                rtv_desc.Format = resource_info[i].format;
                rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                rtv_desc.Texture2D.MipSlice = 0;
                rtv_desc.Texture2D.PlaneSlice = 0;

                descriptor_info->cpu_handle.ptr = 
                        descriptor_info->base_cpu_handle.ptr + i *
                        descriptor_info->stride;

                ID3D12Device_CreateRenderTargetView(device_info->device,
                        resource_info[i].resource, &rtv_desc,
                        descriptor_info->cpu_handle);
        }
}

void create_depthstencil_view(struct gpu_device_info *device_info,
        struct gpu_descriptor_info *descriptor_info,
        struct gpu_resource_info *resource_info)
{
        for (UINT i = 0; i < descriptor_info->num_descriptors; ++i) {
                D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc;
                dsv_desc.Format = resource_info[i].format;
                dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
                dsv_desc.Texture2D.MipSlice = 0;

                descriptor_info->cpu_handle.ptr =
                        descriptor_info->base_cpu_handle.ptr + i *
                        descriptor_info->stride;

                ID3D12Device_CreateDepthStencilView(device_info->device,
                        resource_info[i].resource, &dsv_desc,
                        descriptor_info->cpu_handle);
        }
}

void create_constant_buffer_view(struct gpu_device_info *device_info,
        struct gpu_descriptor_info *descriptor_info,
        struct gpu_resource_info *resource_info)
{
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
        cbv_desc.BufferLocation = resource_info->gpu_address;
        cbv_desc.SizeInBytes = (UINT) resource_info->width;

        ID3D12Device_CreateConstantBufferView(device_info->device, &cbv_desc,
                descriptor_info->cpu_handle);
}

void create_unorderd_access_view(struct gpu_device_info *device_info,
        struct gpu_descriptor_info *descriptor_info,
        struct gpu_resource_info *resource_info)
{
         D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc;
         uav_desc.Format = resource_info->format;
         uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
         switch (uav_desc.ViewDimension)
         {
                case D3D12_UAV_DIMENSION_TEXTURE2D :
                        uav_desc.Texture2D.MipSlice = 0;
                        uav_desc.Texture2D.PlaneSlice = 0;
                        break;

                default :
                        break;
         };
        
         ID3D12Device_CreateUnorderedAccessView(device_info->device,
                resource_info->resource, NULL, &uav_desc,
                descriptor_info->cpu_handle);
}

void create_shader_resource_view(struct gpu_device_info *device_info,
        struct gpu_descriptor_info *descriptor_info,
        struct gpu_resource_info *resource_info)
{
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc;
        srv_desc.Format = resource_info->format;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        switch (srv_desc.ViewDimension)
        {
                case D3D12_SRV_DIMENSION_TEXTURE2D :
                        srv_desc.Texture2D.MostDetailedMip = 0;
                        srv_desc.Texture2D.MipLevels = 1;
                        srv_desc.Texture2D.PlaneSlice = 0;
                        srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;
                        break;

                default :
                        break;
        }

        ID3D12Device_CreateShaderResourceView(device_info->device,
                resource_info->resource, &srv_desc,
                descriptor_info->cpu_handle);
}

void create_sampler(struct gpu_device_info *device_info,
        struct gpu_descriptor_info *descriptor_info)
{
        D3D12_SAMPLER_DESC sampler_desc;
        sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler_desc.MipLODBias = 0;
        sampler_desc.MaxAnisotropy = 0;
        sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler_desc.BorderColor[0] =
                D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler_desc.BorderColor[1] =
                D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler_desc.BorderColor[2] =
                D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler_desc.BorderColor[3] =
                D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler_desc.MinLOD = 0.0f;
        sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;

        ID3D12Device_CreateSampler(device_info->device, &sampler_desc,
                descriptor_info->cpu_handle);
}


void create_cmd_allocators(struct gpu_device_info *device_info,
        struct gpu_cmd_allocator_info *cmd_allocator_info)
{
        cmd_allocator_info->cmd_allocators = malloc(
                cmd_allocator_info->cmd_allocator_count *
                sizeof(ID3D12CommandAllocator *));

        HRESULT result;

        for (UINT i = 0; i < cmd_allocator_info->cmd_allocator_count; ++i) {
                result = ID3D12Device_CreateCommandAllocator(
                        device_info->device, cmd_allocator_info->cmd_list_type,
                        &IID_ID3D12CommandAllocator,
                        &cmd_allocator_info->cmd_allocators[i]);
                show_error_if_failed(result);

                WCHAR name[1024];
                wcscpy(name, cmd_allocator_info->name);
                wcscat(name, L" %d");
                create_wstring(name, name, i);
                result = ID3D12Object_SetName(
                        cmd_allocator_info->cmd_allocators[i], name);
                show_error_if_failed(result);
        }
}

void release_cmd_allocators(struct gpu_cmd_allocator_info *cmd_allocator_info)
{
        for (UINT i = 0; i < cmd_allocator_info->cmd_allocator_count; ++i) {
                ID3D12Device_Release(cmd_allocator_info->cmd_allocators[i]);
        }

        free(cmd_allocator_info->cmd_allocators);
}

void reset_cmd_allocators(struct gpu_cmd_allocator_info *cmd_allocator_info)
{
        HRESULT result;

        for (UINT i = 0; i < cmd_allocator_info->cmd_allocator_count; ++i) {
                result = ID3D12CommandAllocator_Reset(
                        cmd_allocator_info->cmd_allocators[i]);
                show_error_if_failed(result);
        }
}

void reset_cmd_allocator(struct gpu_cmd_allocator_info *cmd_allocator_info,
        UINT index)
{
        HRESULT result;
        result = ID3D12CommandAllocator_Reset(
                cmd_allocator_info->cmd_allocators[index]);
        show_error_if_failed(result);
}


void create_cmd_list(struct gpu_device_info *device_info,
        struct gpu_cmd_allocator_info *cmd_allocator_info,
        struct gpu_cmd_list_info *cmd_list_info)
{
        HRESULT result;

        result = ID3D12Device_CreateCommandList(device_info->device, 0,
                cmd_list_info->cmd_list_type,
                cmd_allocator_info->cmd_allocators[0], NULL,
                &IID_ID3D12GraphicsCommandList, &cmd_list_info->cmd_list);
        show_error_if_failed(result);

        result = ID3D12Object_SetName(
                cmd_list_info->cmd_list, cmd_list_info->name);
        show_error_if_failed(result);
}

void release_cmd_list(struct gpu_cmd_list_info *cmd_list_info)
{
        ID3D12GraphicsCommandList_Release(cmd_list_info->cmd_list);
}

void close_cmd_list(struct gpu_cmd_list_info *cmd_list_info)
{
        ID3D12GraphicsCommandList_Close(cmd_list_info->cmd_list);
}

void execute_cmd_list(struct gpu_cmd_queue_info *cmd_queue_info,
        struct gpu_cmd_list_info *cmd_list_info)
{
        ID3D12CommandList *command_lists[] =
                { (ID3D12CommandList *) cmd_list_info->cmd_list };
        ID3D12CommandQueue_ExecuteCommandLists(cmd_queue_info->cmd_queue,
                _countof(command_lists), command_lists);
}

void reset_cmd_list(struct gpu_cmd_allocator_info *cmd_allocator_info,
        struct gpu_cmd_list_info *cmd_list_info, UINT index)
{
        ID3D12GraphicsCommandList_Reset(cmd_list_info->cmd_list,
                cmd_allocator_info->cmd_allocators[index], NULL);
}

void rec_copy_buffer_region_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_resource_info *dst_resource_info,
        struct gpu_resource_info *src_resource_info)
{
        ID3D12GraphicsCommandList_CopyBufferRegion(
                cmd_list_info->cmd_list, dst_resource_info->resource, 0,
                src_resource_info->resource, 0, src_resource_info->width);
}

void rec_copy_texture_region_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_resource_info *dst_resource_info,
        struct gpu_resource_info *src_resource_info)
{
        D3D12_TEXTURE_COPY_LOCATION dst_tex_loc;
        dst_tex_loc.pResource = dst_resource_info->resource;
        dst_tex_loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst_tex_loc.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION src_tex_loc;
        src_tex_loc.pResource = src_resource_info->resource;
        src_tex_loc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src_tex_loc.PlacedFootprint.Offset = 0;
        src_tex_loc.PlacedFootprint.Footprint.Format =
                dst_resource_info->format;
        src_tex_loc.PlacedFootprint.Footprint.Width =
                (UINT) dst_resource_info->width;
        src_tex_loc.PlacedFootprint.Footprint.Height =
                (UINT) dst_resource_info->height;
        src_tex_loc.PlacedFootprint.Footprint.Depth = 1;
        src_tex_loc.PlacedFootprint.Footprint.RowPitch = 
                (UINT) align_offset(dst_resource_info->width * sizeof (DWORD),
                        D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

        ID3D12GraphicsCommandList_CopyTextureRegion(cmd_list_info->cmd_list,
                &dst_tex_loc, 0, 0, 0, &src_tex_loc, NULL);
}

void rec_copy_resource_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_resource_info *dst_resource_info,
        struct gpu_resource_info *src_resource_info)
{
        ID3D12GraphicsCommandList_CopyResource(cmd_list_info->cmd_list,
                dst_resource_info->resource, src_resource_info->resource);
}

void rec_clear_rtv_cmd(struct gpu_cmd_list_info *cmd_list_info, 
        struct gpu_descriptor_info *rtv_desc_info,
        float *clear_colour)
{
        ID3D12GraphicsCommandList_ClearRenderTargetView(
                cmd_list_info->cmd_list, rtv_desc_info->cpu_handle,
                clear_colour, 0, NULL);
}

void rec_clear_dsv_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_descriptor_info *dsv_desc_info)
{
        ID3D12GraphicsCommandList_ClearDepthStencilView(
                cmd_list_info->cmd_list, dsv_desc_info->cpu_handle, 
                D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
}

void rec_set_pipeline_state_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_pso_info *pso_info)
{
        ID3D12GraphicsCommandList_SetPipelineState(
                cmd_list_info->cmd_list, pso_info->pso);
}

void rec_set_render_target_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_descriptor_info *rtv_desc_info,
        struct gpu_descriptor_info *dsv_desc_info)
{
        ID3D12GraphicsCommandList_OMSetRenderTargets(
                cmd_list_info->cmd_list, 1, &rtv_desc_info->cpu_handle, TRUE,
                &dsv_desc_info->cpu_handle);
}

void rec_set_viewport_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_viewport_info *viewport_info)
{
        ID3D12GraphicsCommandList_RSSetViewports(
                cmd_list_info->cmd_list, 1, &viewport_info->viewport);
}

void rec_set_scissor_rect_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_scissor_rect_info *scissor_rect_info)
{
        ID3D12GraphicsCommandList_RSSetScissorRects(
                cmd_list_info->cmd_list, 1, &scissor_rect_info->scissor_rect);
}

void rec_set_primitive_cmd(struct gpu_cmd_list_info *cmd_list_info,
        D3D_PRIMITIVE_TOPOLOGY prim_type)
{
        ID3D12GraphicsCommandList_IASetPrimitiveTopology(
                cmd_list_info->cmd_list, prim_type);
}

void rec_set_compute_root_sig_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_root_sig_info *root_sig_info)
{
        ID3D12GraphicsCommandList_SetComputeRootSignature(
                cmd_list_info->cmd_list, root_sig_info->root_sig);
}

void rec_set_graphics_root_sig_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_root_sig_info *root_sig_info)
{
        ID3D12GraphicsCommandList_SetGraphicsRootSignature(
                cmd_list_info->cmd_list, root_sig_info->root_sig);
}

void rec_set_descriptor_heap_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_descriptor_info *descriptor_info)
{
        ID3D12GraphicsCommandList_SetDescriptorHeaps(
                cmd_list_info->cmd_list, 1, &descriptor_info->descriptor_heap);
}

void rec_set_compute_root_descriptor_table_cmd(
        struct gpu_cmd_list_info *cmd_list_info, UINT root_param_index,
        struct gpu_descriptor_info *descriptor_info)
{
        ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(
                cmd_list_info->cmd_list, root_param_index, 
                descriptor_info->gpu_handle);
}

void rec_set_graphics_root_descriptor_table_cmd(
        struct gpu_cmd_list_info *cmd_list_info, UINT root_param_index,
        struct gpu_descriptor_info *descriptor_info)
{
        ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(
                cmd_list_info->cmd_list, root_param_index,
                descriptor_info->gpu_handle);
}

void rec_set_vertex_buffer_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_resource_info *vert_buffer_info, UINT stride)
{
        D3D12_VERTEX_BUFFER_VIEW vert_buffer_view;
        vert_buffer_view.BufferLocation = vert_buffer_info->gpu_address;
        vert_buffer_view.SizeInBytes = (UINT) vert_buffer_info->width;
        vert_buffer_view.StrideInBytes = stride;

        ID3D12GraphicsCommandList_IASetVertexBuffers(
                cmd_list_info->cmd_list, 0, 1, &vert_buffer_view);
}

void rec_set_index_buffer_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_resource_info *index_buffer)
{
        D3D12_INDEX_BUFFER_VIEW index_buffer_view;
        index_buffer_view.BufferLocation = index_buffer->gpu_address;
        index_buffer_view.SizeInBytes = (UINT) index_buffer->width;
        index_buffer_view.Format = DXGI_FORMAT_R32_UINT;

        ID3D12GraphicsCommandList_IASetIndexBuffer(
                cmd_list_info->cmd_list, &index_buffer_view);
}

void rec_dispatch_cmd(struct gpu_cmd_list_info *cmd_list_info,
        UINT thread_group_coun_x, UINT thread_group_coun_y,
        UINT thread_group_coun_z)
{
        ID3D12GraphicsCommandList_Dispatch(cmd_list_info->cmd_list,
                thread_group_coun_x, thread_group_coun_y, thread_group_coun_z);
}

void rec_draw_indexed_instance_cmd(struct gpu_cmd_list_info *cmd_list_info,
        UINT index_count, UINT instance_count)
{
        ID3D12GraphicsCommandList_DrawIndexedInstanced(
                cmd_list_info->cmd_list, index_count, instance_count, 0, 0, 0);
}

void transition_resources(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_resource_info **resource_info_list,
        D3D12_RESOURCE_STATES *resource_end_state_list, UINT resource_count)
{
        #define MAX_RESC_BARRIERS 2
        D3D12_RESOURCE_BARRIER resource_barrier[MAX_RESC_BARRIERS];
        for (UINT i = 0; i < resource_count; ++i) {
                struct gpu_resource_info *resource_info =
                        resource_info_list[i];
                resource_barrier[i].Type =
                        D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                resource_barrier[i].Flags =
                        D3D12_RESOURCE_BARRIER_FLAG_NONE;
                resource_barrier[i].Transition.pResource =
                        resource_info->resource;
                resource_barrier[i].Transition.Subresource =
                        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                resource_barrier[i].Transition.StateBefore =
                        resource_info->current_state;
                resource_barrier[i].Transition.StateAfter =
                        resource_end_state_list[i];
        }

        ID3D12GraphicsCommandList_ResourceBarrier(
                cmd_list_info->cmd_list, resource_count, resource_barrier);

        for (UINT i = 0; i < resource_count; ++i) {
                 struct gpu_resource_info *resource_info =
                        resource_info_list[i];
                resource_info->current_state = resource_end_state_list[i];
        }
}


void create_fence(struct gpu_device_info *device_info, 
        struct gpu_fence_info *fence_info)
{
        fence_info->fence_values = malloc(fence_info->num_fence_value *
                sizeof (UINT64));
        for (UINT i = 0; i < fence_info->num_fence_value; ++i) {
                fence_info->fence_values[i] = 0;
        }

        fence_info->cur_fence_value = 0;

        HRESULT result;

        result = ID3D12Device_CreateFence(device_info->device,
                fence_info->cur_fence_value, D3D12_FENCE_FLAG_NONE,
                &IID_ID3D12Fence, &fence_info->fence);
        show_error_if_failed(result);

        result = ID3D12Object_SetName(fence_info->fence, fence_info->name);
        show_error_if_failed(result);

        fence_info->fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
        assert(fence_info->fence_event);
}

void release_fence(struct gpu_fence_info *fence_info)
{
        CloseHandle(fence_info->fence_event);

        fence_info->fence->lpVtbl->Release(fence_info->fence);

        free(fence_info->fence_values);
}

void signal_gpu(struct gpu_cmd_queue_info *cmd_queue_info,
        struct gpu_fence_info *fence_info, UINT index)
{
        fence_info->fence_values[index] = ++(fence_info->cur_fence_value);

        HRESULT result;

        result = cmd_queue_info->cmd_queue->lpVtbl->Signal(
                cmd_queue_info->cmd_queue, fence_info->fence,
                fence_info->fence_values[index]);

        show_error_if_failed(result);
}

void wait_for_fence(struct gpu_cmd_queue_info *cmd_queue_info,
                   struct gpu_fence_info *fence_info, UINT index)
{
        UINT64 fence_val = fence_info->fence_values[index];

        HRESULT result;

        result = cmd_queue_info->cmd_queue->lpVtbl->Wait(
                cmd_queue_info->cmd_queue, fence_info->fence,
                fence_val);

        show_error_if_failed(result);
}

void wait_for_gpu(struct gpu_fence_info *fence_info, UINT index)
{
        UINT64 fence_val = fence_info->fence_values[index];

        if (ID3D12Fence_GetCompletedValue(fence_info->fence) < fence_val) {
                HRESULT result;

                result = ID3D12Fence_SetEventOnCompletion(fence_info->fence,
                        fence_val, fence_info->fence_event);
                show_error_if_failed(result);
                WaitForSingleObject(fence_info->fence_event, INFINITE);
        }
}


void compile_shader(struct gpu_shader_info *shader_info)
{
        #if defined(_DEBUG)
        shader_info->flags = D3DCOMPILE_DEBUG | 
                             D3DCOMPILE_SKIP_OPTIMIZATION | 
                             D3DCOMPILE_WARNINGS_ARE_ERRORS;
        #else
        shader_info->flags = 0;
        #endif

        HRESULT result;

        ID3DBlob *shader_error_blob = NULL;
        shader_info->shader_blob = NULL;

        result = D3DCompileFromFile(shader_info->shader_file, NULL, NULL,
                "main", shader_info->shader_target, shader_info->flags, 0,
                &shader_info->shader_blob, &shader_error_blob);
        show_error_if_failed(result);
        assert(shader_error_blob == NULL);

        shader_info->shader_byte_code = ID3D10Blob_GetBufferPointer(
                shader_info->shader_blob);

        shader_info->shader_byte_code_len = ID3D10Blob_GetBufferSize(
                shader_info->shader_blob);
}

void release_shader(struct gpu_shader_info *shader_info)
{
        ID3D10Blob_Release(shader_info->shader_blob);
}


void setup_vertex_input(LPCSTR *attribute_names, DXGI_FORMAT *attribute_formats, 
                       struct gpu_vert_input_info *input_info)
{
        input_info->input_element_descs = malloc(input_info->attribute_count *
                sizeof (D3D12_INPUT_ELEMENT_DESC));
        for (UINT i = 0; i < input_info->attribute_count; ++i) {
                input_info->input_element_descs[i].SemanticName =
                        attribute_names[i];
                input_info->input_element_descs[i].SemanticIndex = 0;
                input_info->input_element_descs[i].Format =
                        attribute_formats[i];
                input_info->input_element_descs[i].InputSlot = 0;
                input_info->input_element_descs[i].AlignedByteOffset =
                        D3D12_APPEND_ALIGNED_ELEMENT;
                input_info->input_element_descs[i].InputSlotClass =
                        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                input_info->input_element_descs[i].InstanceDataStepRate = 0;
        }
}

void free_vertex_input(struct gpu_vert_input_info *input_info)
{
        free(input_info->input_element_descs);
}


void create_root_sig(struct gpu_device_info *device_info,
        struct gpu_root_param_info *root_param_infos, UINT num_root_params,
        struct gpu_root_sig_info *root_sig_info)
{
        D3D12_DESCRIPTOR_RANGE *descriptor_ranges = malloc(num_root_params *
                sizeof (D3D12_DESCRIPTOR_RANGE));

        D3D12_ROOT_PARAMETER *root_params = malloc(num_root_params *
                sizeof (D3D12_ROOT_PARAMETER));

        for (UINT i = 0; i < num_root_params; ++i) {
                descriptor_ranges[i].RangeType =
                        root_param_infos[i].range_type;
                descriptor_ranges[i].NumDescriptors =
                        root_param_infos[i].num_descriptors;
                descriptor_ranges[i].BaseShaderRegister = 0;
                descriptor_ranges[i].RegisterSpace = 0;
                descriptor_ranges[i].OffsetInDescriptorsFromTableStart = 0;

                root_params[i].ParameterType =
                        D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                root_params[i].DescriptorTable.NumDescriptorRanges = 1;
                root_params[i].DescriptorTable.pDescriptorRanges =
                        &descriptor_ranges[i];
                root_params[i].ShaderVisibility =
                        root_param_infos[i].shader_visbility;
        }

        D3D12_ROOT_SIGNATURE_FLAGS root_sig_flags =
                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc;
        root_sig_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
        root_sig_desc.Desc_1_0.NumParameters = num_root_params;
        root_sig_desc.Desc_1_0.pParameters = root_params;
        root_sig_desc.Desc_1_0.NumStaticSamplers = 0;
        root_sig_desc.Desc_1_0.pStaticSamplers = NULL;
        root_sig_desc.Desc_1_0.Flags = root_sig_flags;

        HRESULT result;

        ID3DBlob *root_sig_error_blob = NULL;
        root_sig_info->root_sig_blob = NULL;
        result = D3D12SerializeVersionedRootSignature(&root_sig_desc,
                &root_sig_info->root_sig_blob, &root_sig_error_blob);
        show_error_if_failed(result);
        assert(root_sig_error_blob == NULL);

        free(root_params);
        free(descriptor_ranges);

        void *root_sig_blob_ptr = ID3D10Blob_GetBufferPointer(
                root_sig_info->root_sig_blob);

        size_t root_sig_blob_len = ID3D10Blob_GetBufferSize(
                root_sig_info->root_sig_blob);

        result = ID3D12Device_CreateRootSignature(device_info->device, 0,
                root_sig_blob_ptr, root_sig_blob_len, &IID_ID3D12RootSignature,
                &root_sig_info->root_sig);
        show_error_if_failed(result);

        result = ID3D12Object_SetName(root_sig_info->root_sig, root_sig_info->name);
        show_error_if_failed(result);
}

void release_root_sig(struct gpu_root_sig_info *root_sig_info)
{
        ID3D12RootSignature_Release(root_sig_info->root_sig);
        ID3D10Blob_Release(root_sig_info->root_sig_blob);
}


void create_pso(struct gpu_device_info *device_info,
        struct gpu_vert_input_info *vert_input_info,
        struct gpu_root_sig_info *root_sig_info, struct gpu_pso_info *pso_info)
{
        switch(pso_info->type)
        {
                case PSO_TYPE_GRAPHICS :
                        create_graphics_pso(device_info, vert_input_info,
                                root_sig_info, pso_info);
                        break;

                case PSO_TYPE_COMPUTE :
                        create_compute_pso(device_info, root_sig_info, 
                                pso_info);
                        break;

                default : 
                        break;
        }

        HRESULT result;
        result = ID3D12Object_SetName(pso_info->pso, pso_info->name);
        show_error_if_failed(result);
}

static void create_graphics_pso(struct gpu_device_info *device_info,
        struct gpu_vert_input_info *vert_input_info,
        struct gpu_root_sig_info *root_sig_info, struct gpu_pso_info *pso_info)
{
        D3D12_GRAPHICS_PIPELINE_STATE_DESC graphics_pso_desc;
        graphics_pso_desc.pRootSignature = root_sig_info->root_sig;
        graphics_pso_desc.VS.pShaderBytecode =
                pso_info->graphics_pso_info.vert_shader_byte_code;
        graphics_pso_desc.VS.BytecodeLength =
                pso_info->graphics_pso_info.vert_shader_byte_code_len;
        graphics_pso_desc.PS.pShaderBytecode =
                pso_info->graphics_pso_info.pix_shader_byte_code;
        graphics_pso_desc.PS.BytecodeLength =
                pso_info->graphics_pso_info.pix_shader_byte_code_len;
        graphics_pso_desc.DS.pShaderBytecode =
                pso_info->graphics_pso_info.dom_shader_byte_code;
        graphics_pso_desc.DS.BytecodeLength =
                pso_info->graphics_pso_info.dom_shader_byte_code_len;
        graphics_pso_desc.HS.pShaderBytecode =
                pso_info->graphics_pso_info.hull_shader_byte_code;
        graphics_pso_desc.HS.BytecodeLength =
                pso_info->graphics_pso_info.hull_shader_byte_code_len;
        graphics_pso_desc.GS.pShaderBytecode =
                pso_info->graphics_pso_info.geom_shader_byte_code;
        graphics_pso_desc.GS.BytecodeLength =
                pso_info->graphics_pso_info.geom_shader_byte_code_len;
        graphics_pso_desc.StreamOutput.pSODeclaration = NULL;
        graphics_pso_desc.StreamOutput.NumEntries = 0;
        graphics_pso_desc.StreamOutput.pBufferStrides = NULL;
        graphics_pso_desc.StreamOutput.NumStrides = 0;
        graphics_pso_desc.StreamOutput.RasterizedStream = 0;
        graphics_pso_desc.BlendState.AlphaToCoverageEnable = FALSE;
        graphics_pso_desc.BlendState.IndependentBlendEnable = FALSE;
        graphics_pso_desc.BlendState.RenderTarget[0].BlendEnable = FALSE;
        graphics_pso_desc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
        graphics_pso_desc.BlendState.RenderTarget[0].SrcBlend =
                D3D12_BLEND_ONE;
        graphics_pso_desc.BlendState.RenderTarget[0].DestBlend =
                D3D12_BLEND_ZERO;
        graphics_pso_desc.BlendState.RenderTarget[0].BlendOp =
                D3D12_BLEND_OP_ADD;
        graphics_pso_desc.BlendState.RenderTarget[0].SrcBlendAlpha =
                D3D12_BLEND_ONE;
        graphics_pso_desc.BlendState.RenderTarget[0].DestBlendAlpha =
                D3D12_BLEND_ZERO;
        graphics_pso_desc.BlendState.RenderTarget[0].BlendOpAlpha =
                D3D12_BLEND_OP_ADD;
        graphics_pso_desc.BlendState.RenderTarget[0].LogicOp =
                D3D12_LOGIC_OP_NOOP;
        graphics_pso_desc.BlendState.RenderTarget[0].RenderTargetWriteMask =
                D3D12_COLOR_WRITE_ENABLE_ALL;
        graphics_pso_desc.SampleMask = UINT_MAX;
        graphics_pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        graphics_pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        graphics_pso_desc.RasterizerState.FrontCounterClockwise = FALSE;
        graphics_pso_desc.RasterizerState.DepthBias = 0;
        graphics_pso_desc.RasterizerState.DepthBiasClamp = 0.0f;
        graphics_pso_desc.RasterizerState.SlopeScaledDepthBias = 0.0f;
        graphics_pso_desc.RasterizerState.DepthClipEnable = TRUE;
        graphics_pso_desc.RasterizerState.MultisampleEnable = FALSE;
        graphics_pso_desc.RasterizerState.AntialiasedLineEnable = FALSE;
        graphics_pso_desc.RasterizerState.ForcedSampleCount = 0;
        graphics_pso_desc.RasterizerState.ConservativeRaster =
                D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        graphics_pso_desc.DepthStencilState.DepthEnable = TRUE;
        graphics_pso_desc.DepthStencilState.DepthWriteMask =
                D3D12_DEPTH_WRITE_MASK_ALL;
        graphics_pso_desc.DepthStencilState.DepthFunc =
                D3D12_COMPARISON_FUNC_LESS_EQUAL;
        graphics_pso_desc.DepthStencilState.StencilEnable = FALSE;
        graphics_pso_desc.DepthStencilState.StencilReadMask =
                D3D12_DEFAULT_STENCIL_READ_MASK;
        graphics_pso_desc.DepthStencilState.StencilWriteMask =
                D3D12_DEFAULT_STENCIL_WRITE_MASK;
        graphics_pso_desc.DepthStencilState.FrontFace.StencilFailOp =
                D3D12_STENCIL_OP_KEEP;
        graphics_pso_desc.DepthStencilState.FrontFace.StencilDepthFailOp =
                D3D12_STENCIL_OP_KEEP;
        graphics_pso_desc.DepthStencilState.FrontFace.StencilPassOp =
                D3D12_STENCIL_OP_KEEP;
        graphics_pso_desc.DepthStencilState.FrontFace.StencilFunc =
                D3D12_COMPARISON_FUNC_ALWAYS;
        graphics_pso_desc.DepthStencilState.BackFace.StencilFailOp =
                D3D12_STENCIL_OP_KEEP;
        graphics_pso_desc.DepthStencilState.BackFace.StencilDepthFailOp =
                D3D12_STENCIL_OP_KEEP;
        graphics_pso_desc.DepthStencilState.BackFace.StencilPassOp =
                D3D12_STENCIL_OP_KEEP;
        graphics_pso_desc.DepthStencilState.BackFace.StencilFunc =
                D3D12_COMPARISON_FUNC_ALWAYS;
        graphics_pso_desc.InputLayout.pInputElementDescs =
                vert_input_info->input_element_descs;
        graphics_pso_desc.InputLayout.NumElements =
                vert_input_info->attribute_count;
        graphics_pso_desc.IBStripCutValue =
                D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
        graphics_pso_desc.PrimitiveTopologyType =
                D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        graphics_pso_desc.NumRenderTargets = 1;
        graphics_pso_desc.RTVFormats[0] =
                pso_info->graphics_pso_info.render_target_format;
        for (UINT i = 1; i < 8; ++i) {
                graphics_pso_desc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
        }
        graphics_pso_desc.DSVFormat =
                pso_info->graphics_pso_info.depth_target_format;
        graphics_pso_desc.SampleDesc.Count = 1;
        graphics_pso_desc.SampleDesc.Quality = 0;
        graphics_pso_desc.NodeMask = 0;
        graphics_pso_desc.CachedPSO.pCachedBlob = NULL;
        graphics_pso_desc.CachedPSO.CachedBlobSizeInBytes = 0;
        graphics_pso_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        HRESULT result;

        result = ID3D12Device_CreateGraphicsPipelineState(device_info->device,
                &graphics_pso_desc, &IID_ID3D12PipelineState, &pso_info->pso);
        
        show_error_if_failed(result);
}

static void create_compute_pso(struct gpu_device_info *device_info,
        struct gpu_root_sig_info *root_sig_info, struct gpu_pso_info *pso_info)
{
        D3D12_COMPUTE_PIPELINE_STATE_DESC compute_pso_desc;
        compute_pso_desc.pRootSignature = root_sig_info->root_sig;
        compute_pso_desc.CS.pShaderBytecode =
                pso_info->compute_pso_info.comp_shader_byte_code;
        compute_pso_desc.CS.BytecodeLength =
                pso_info->compute_pso_info.comp_shader_byte_code_len;
        compute_pso_desc.NodeMask = 0;
        compute_pso_desc.CachedPSO.pCachedBlob = NULL;
        compute_pso_desc.CachedPSO.CachedBlobSizeInBytes = 0;
        compute_pso_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        HRESULT result;

        result = ID3D12Device_CreateComputePipelineState(device_info->device,
                &compute_pso_desc, &IID_ID3D12PipelineState, &pso_info->pso);

        show_error_if_failed(result);
}

void release_pso(struct gpu_pso_info *pso_info)
{
        ID3D12PipelineState_Release(pso_info->pso);
}


void create_viewport(struct gpu_viewport_info *viewport_info)
{
        viewport_info->viewport.TopLeftX = 0.0f;
        viewport_info->viewport.TopLeftY = 0.0f;
        viewport_info->viewport.Width = viewport_info->width;
        viewport_info->viewport.Height = viewport_info->height;
        viewport_info->viewport.MinDepth = D3D12_MIN_DEPTH;
        viewport_info->viewport.MaxDepth = D3D12_MAX_DEPTH;
}


void create_scissor_rect(struct gpu_scissor_rect_info *scissor_rect_info)
{
        scissor_rect_info->scissor_rect.left = 0;
        scissor_rect_info->scissor_rect.top = 0;
        scissor_rect_info->scissor_rect.right = LONG_MAX;
        scissor_rect_info->scissor_rect.bottom = LONG_MAX;
}