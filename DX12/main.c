#include "window_interface.h"
#include "gpu_interface.h"
#include "swapchain_inerface.h"
#include "camera_interface.h"
#include "material_interface.h"
#include "error.h"
#include "misc.h"
#include "gltf_interface.h"

#define CGLTF_IMPLEMENTATION
#include "libs/cgltf/cgltf.h"


int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
        _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
        struct window_info wnd_info;
        wnd_info.window_class_name = "DX12WindowClass";
        wnd_info.window_name = "DX12Window";
        wnd_info.x = 100;
        wnd_info.y = 100;
        wnd_info.width = 800;
        wnd_info.height = 600;
        create_window(&wnd_info, hInstance, nCmdShow);

        // Create device
        struct gpu_device_info device_info;
        create_gpu_device(&device_info);

        // Create render queue
        struct gpu_cmd_queue_info render_queue_info;
        create_wstring(render_queue_info.name, L"Render Queue");
        render_queue_info.type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        create_cmd_queue(&device_info, &render_queue_info);

        // Create compute queue
        struct gpu_cmd_queue_info compute_queue_info;
        create_wstring(compute_queue_info.name, L"Compute Queue");
        compute_queue_info.type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        create_cmd_queue(&device_info, &compute_queue_info);

        // Create copy queue
        struct gpu_cmd_queue_info copy_queue_info;
        create_wstring(copy_queue_info.name,L"Copy Queue");
        copy_queue_info.type = D3D12_COMMAND_LIST_TYPE_COPY;
        create_cmd_queue(&device_info, &copy_queue_info);

        // Create present queue
        struct gpu_cmd_queue_info present_queue_info;
        create_wstring(present_queue_info.name, L"Present Queue");
        present_queue_info.type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        create_cmd_queue(&device_info, &present_queue_info);

        // Create swapchain
        struct swapchain_info swp_chain_info;
        swp_chain_info.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        swp_chain_info.buffer_count = 2;
        create_swapchain(&wnd_info, &device_info, &present_queue_info,
                &swp_chain_info);

        // Create swapchain render target descriptor
        struct gpu_descriptor_info rtv_descriptor_info;
        create_wstring(rtv_descriptor_info.name, L"Swapchain RTV heap");
        rtv_descriptor_info.type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtv_descriptor_info.num_descriptors = swp_chain_info.buffer_count;
        rtv_descriptor_info.flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        create_descriptor(&device_info, &rtv_descriptor_info);

        // Get swapchain render target resource
        struct gpu_resource_info *rtv_resource_info;
        rtv_resource_info = malloc(
                rtv_descriptor_info.num_descriptors *
                sizeof (struct gpu_resource_info));

        for (UINT i = 0; i < rtv_descriptor_info.num_descriptors; ++i) {
                rtv_resource_info[i].resource = get_swapchain_buffer(
                        &swp_chain_info, i);
                rtv_resource_info[i].format = swp_chain_info.format;
                rtv_resource_info[i].current_state =
                        D3D12_RESOURCE_STATE_PRESENT;
        }

        // Create swapchain render target view
        struct gpu_view_info rtv_view_info;
        rtv_view_info.rtv_dimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        create_rendertarget_view(&device_info, &rtv_descriptor_info,
                rtv_resource_info, &rtv_view_info);

        // Create intermediate render target descriptor
        struct gpu_descriptor_info tmp_rtv_descriptor_info;
        create_wstring(tmp_rtv_descriptor_info.name, L"TMP RTV heap");
        tmp_rtv_descriptor_info.type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        tmp_rtv_descriptor_info.num_descriptors = swp_chain_info.buffer_count;
        tmp_rtv_descriptor_info.flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        create_descriptor(&device_info, &tmp_rtv_descriptor_info);

        // Create intermediate render target resource
        struct gpu_resource_info *tmp_rtv_resource_info;
        tmp_rtv_resource_info = malloc(
                tmp_rtv_descriptor_info.num_descriptors *
                sizeof (struct gpu_resource_info));
        for (UINT i = 0; i < tmp_rtv_descriptor_info.num_descriptors; ++i) {
                tmp_rtv_resource_info[i].type = D3D12_HEAP_TYPE_DEFAULT;
                tmp_rtv_resource_info[i].dimension =
                        D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                tmp_rtv_resource_info[i].width = wnd_info.width;
                tmp_rtv_resource_info[i].height = wnd_info.height;
                tmp_rtv_resource_info[i].mip_levels = 1;
                tmp_rtv_resource_info[i].format = swp_chain_info.format;
                tmp_rtv_resource_info[i].layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
                tmp_rtv_resource_info[i].flags =
                        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET |
                        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                tmp_rtv_resource_info[i].current_state =
                        D3D12_RESOURCE_STATE_RENDER_TARGET;
                create_wstring(tmp_rtv_resource_info[i].name,
                        L"TMP RTV Resource %d", i);
                create_resource(&device_info, &tmp_rtv_resource_info[i]);
        }

        // Create intermediate render target view
        struct gpu_view_info tmp_rtv_view_info;
        tmp_rtv_view_info.rtv_dimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        create_rendertarget_view(&device_info, &tmp_rtv_descriptor_info,
                 tmp_rtv_resource_info, &tmp_rtv_view_info);

        // Create command allocators for draw commands
        struct gpu_cmd_allocator_info render_cmd_allocator_info;
        create_wstring(render_cmd_allocator_info.name, L"Render Cmd alloc");
        render_cmd_allocator_info.cmd_list_type =
                D3D12_COMMAND_LIST_TYPE_DIRECT;
        render_cmd_allocator_info.cmd_allocator_count =
                swp_chain_info.buffer_count;
        create_cmd_allocators(&device_info, &render_cmd_allocator_info);

        // Create command list for draw commands
        struct gpu_cmd_list_info render_cmd_list_info;
        create_wstring(render_cmd_list_info.name, L"Render Cmd list");
        render_cmd_list_info.cmd_list_type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        create_cmd_list(&device_info, &render_cmd_allocator_info, 
                &render_cmd_list_info);

        // Create command allocators for copy commands
        struct gpu_cmd_allocator_info copy_cmd_allocator_info;
        create_wstring(copy_cmd_allocator_info.name, L"Copy Cmd alloc");
        copy_cmd_allocator_info.cmd_list_type = D3D12_COMMAND_LIST_TYPE_COPY;
        copy_cmd_allocator_info.cmd_allocator_count = 1;
        create_cmd_allocators(&device_info, &copy_cmd_allocator_info);

        // Create command list for copy commands
        struct gpu_cmd_list_info copy_cmd_list_info;
        create_wstring(copy_cmd_list_info.name, L"Copy Cmd list");
        copy_cmd_list_info.cmd_list_type = D3D12_COMMAND_LIST_TYPE_COPY;
        create_cmd_list(&device_info, &copy_cmd_allocator_info,
                &copy_cmd_list_info);

        // Create command allocators for compute commands
        struct gpu_cmd_allocator_info compute_cmd_allocator_info;
        create_wstring(compute_cmd_allocator_info.name, L"Compute Cmd alloc");
        compute_cmd_allocator_info.cmd_list_type =
                D3D12_COMMAND_LIST_TYPE_COMPUTE;
        compute_cmd_allocator_info.cmd_allocator_count = 2;
        create_cmd_allocators(&device_info, &compute_cmd_allocator_info);

        // Create command list for compute commands
        struct gpu_cmd_list_info compute_cmd_list_info;
        create_wstring(compute_cmd_list_info.name, L"Compute Cmd list");
        compute_cmd_list_info.cmd_list_type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        create_cmd_list(&device_info, &compute_cmd_allocator_info,
                &compute_cmd_list_info);

        // Create command allocator for present queue
        struct gpu_cmd_allocator_info present_cmd_allocator_info;
        create_wstring(present_cmd_allocator_info.name, L"Present Cmd alloc");
        present_cmd_allocator_info.cmd_allocator_count =
                swp_chain_info.buffer_count;
        present_cmd_allocator_info.cmd_list_type =
                D3D12_COMMAND_LIST_TYPE_DIRECT;
        create_cmd_allocators(&device_info, &present_cmd_allocator_info);

        // Create command list for present queue
        struct gpu_cmd_list_info present_cmd_list_info;
        create_wstring(present_cmd_list_info.name, L"Present Cmd list");
        present_cmd_list_info.cmd_list_type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        create_cmd_list(&device_info, &present_cmd_allocator_info,
                &present_cmd_list_info);

        // Create fence for synchronizing
        struct gpu_fence_info fence_info;
        create_wstring(fence_info.name, L"Fence");
        fence_info.num_fence_value = swp_chain_info.buffer_count;
        create_fence(&device_info, &fence_info);

        // GLTF
        struct cgltf_data *gltf_pdata;
        create_gltf("media\\buster_drone\\busterDrone.gltf", &gltf_pdata);

        struct gltf_node_info *node_info;
        size_t gltf_node_count = gltf_pdata->nodes_count;
        size_t gltf_mesh_count = gltf_pdata->meshes_count;

        // I only want to create renderable nodes
        node_info = malloc(gltf_mesh_count * sizeof (struct gltf_node_info));
        // But want to process all nodes
        process_gltf_nodes(gltf_node_count, node_info, gltf_pdata->nodes);

        struct gpu_resource_info *vert_gpu_resource_info;
        struct gpu_resource_info *vert_upload_resource_info;
        struct gpu_resource_info *indices_gpu_resource_info;
        struct gpu_resource_info *indices_upload_resource_info;

        vert_gpu_resource_info = malloc(gltf_mesh_count *
                sizeof (struct gpu_resource_info));
        vert_upload_resource_info = malloc(gltf_mesh_count *
                sizeof (struct gpu_resource_info));
        indices_gpu_resource_info = malloc(gltf_mesh_count *
                sizeof (struct gpu_resource_info));
        indices_upload_resource_info = malloc(gltf_mesh_count *
                sizeof (struct gpu_resource_info));

        struct gpu_vert_input_info *vert_input_info;
        vert_input_info = malloc(gltf_mesh_count *
                sizeof (struct gpu_vert_input_info));

        for (UINT i = 0; i < gltf_mesh_count; ++i) {

                struct gltf_mesh_info *mesh_info = node_info[i].mesh_info;

                struct gltf_primitive_info *primitive_info;
                primitive_info = &mesh_info->primitive_info;

                struct gltf_attribute_info *attribute_info;
                attribute_info = &primitive_info->attribute_info;

                // Create vertex resource
                // First resource for vertices on the GPU for shader usage
                create_wstring(vert_gpu_resource_info[i].name,
                        L"Vert GPU resource %d", i);
                vert_gpu_resource_info[i].type = D3D12_HEAP_TYPE_DEFAULT;
                vert_gpu_resource_info[i].dimension =
                        D3D12_RESOURCE_DIMENSION_BUFFER;
                vert_gpu_resource_info[i].width =
                        attribute_info->attribute_data_size;
                vert_gpu_resource_info[i].height = 1;
                vert_gpu_resource_info[i].mip_levels = 1;
                vert_gpu_resource_info[i].format = DXGI_FORMAT_UNKNOWN;
                vert_gpu_resource_info[i].layout =
                        D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                vert_gpu_resource_info[i].flags = D3D12_RESOURCE_FLAG_NONE;
                vert_gpu_resource_info[i].current_state =
                        D3D12_RESOURCE_STATE_COMMON;
                create_resource(&device_info, &vert_gpu_resource_info[i]);

                // Resource for uploading vertices from CPU to GPU
                create_wstring(vert_upload_resource_info[i].name,
                        L"Vert upload resource %d", i);
                vert_upload_resource_info[i].type = D3D12_HEAP_TYPE_UPLOAD;
                vert_upload_resource_info[i].dimension =
                        D3D12_RESOURCE_DIMENSION_BUFFER;
                vert_upload_resource_info[i].width =
                        attribute_info->attribute_data_size;
                vert_upload_resource_info[i].height = 1;
                vert_upload_resource_info[i].mip_levels = 1;
                vert_upload_resource_info[i].format = DXGI_FORMAT_UNKNOWN;
                vert_upload_resource_info[i].layout =
                        D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                vert_upload_resource_info[i].flags = D3D12_RESOURCE_FLAG_NONE;
                vert_upload_resource_info[i].current_state =
                        D3D12_RESOURCE_STATE_GENERIC_READ;
                create_resource(&device_info, &vert_upload_resource_info[i]);

                // Copy triangle vertex data from cpu to upload resource
                upload_resources(&vert_upload_resource_info[i],
                        attribute_info->attribute_data);

                // Copy vertex data from upload resource to gpu shader resource
                rec_copy_buffer_region_cmd(&copy_cmd_list_info,
                        &vert_gpu_resource_info[i],
                        &vert_upload_resource_info[i]);

                // Resource for index buffer on the GPU for shader usage
                create_wstring(indices_gpu_resource_info[i].name,
                        L"Indices GPU resource %d", i);
                indices_gpu_resource_info[i].type = D3D12_HEAP_TYPE_DEFAULT;
                indices_gpu_resource_info[i].dimension =
                        D3D12_RESOURCE_DIMENSION_BUFFER;
                indices_gpu_resource_info[i].width = primitive_info->index_size;
                indices_gpu_resource_info[i].height = 1;
                indices_gpu_resource_info[i].mip_levels = 1;
                indices_gpu_resource_info[i].format = DXGI_FORMAT_UNKNOWN;
                indices_gpu_resource_info[i].layout =
                        D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                indices_gpu_resource_info[i].flags = D3D12_RESOURCE_FLAG_NONE;
                indices_gpu_resource_info[i].current_state =
                        D3D12_RESOURCE_STATE_COMMON;
                create_resource(&device_info, &indices_gpu_resource_info[i]);

                // Resource for uploading indices from CPU to GPU
                create_wstring(indices_upload_resource_info[i].name,
                        L"Indices upload resource %d", i);
                indices_upload_resource_info[i].type = D3D12_HEAP_TYPE_UPLOAD;
                indices_upload_resource_info[i].dimension =
                        D3D12_RESOURCE_DIMENSION_BUFFER;
                indices_upload_resource_info[i].width =
                        primitive_info->index_size;
                indices_upload_resource_info[i].height = 1;
                indices_upload_resource_info[i].mip_levels = 1;
                indices_upload_resource_info[i].format = DXGI_FORMAT_UNKNOWN;
                indices_upload_resource_info[i].layout =
                        D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                indices_upload_resource_info[i].flags =
                        D3D12_RESOURCE_FLAG_NONE;
                indices_upload_resource_info[i].current_state =
                        D3D12_RESOURCE_STATE_GENERIC_READ;
                create_resource(&device_info, &indices_upload_resource_info[i]);

                upload_resources(&indices_upload_resource_info[i],
                        primitive_info->index_data);

                rec_copy_buffer_region_cmd(&copy_cmd_list_info,
                        &indices_gpu_resource_info[i],
                        &indices_upload_resource_info[i]);

                vert_input_info[i].attribute_count =
                        (UINT) attribute_info->attribute_type_count;
                setup_vertex_input(attribute_info->attribute_type_names,
                attribute_info->attribute_type_data_format, &vert_input_info[i]);
        }

        // Create depth buffer descriptor 
        struct gpu_descriptor_info dsv_descriptor_info;
        create_wstring(dsv_descriptor_info.name, L"DSV Heap");
        dsv_descriptor_info.type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsv_descriptor_info.num_descriptors = swp_chain_info.buffer_count;
        dsv_descriptor_info.flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        create_descriptor(&device_info, &dsv_descriptor_info);

        // Create depth buffer resource
        struct gpu_resource_info *dsv_resource_info;
        dsv_resource_info = malloc(dsv_descriptor_info.num_descriptors *
                sizeof (struct gpu_resource_info));
        for (UINT i = 0; i < dsv_descriptor_info.num_descriptors; ++i) {
                create_wstring(dsv_resource_info[i].name, L"DSV resource %d");
                dsv_resource_info[i].type = D3D12_HEAP_TYPE_DEFAULT;
                dsv_resource_info[i].dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                dsv_resource_info[i].width = wnd_info.width;
                dsv_resource_info[i].height = wnd_info.height;
                dsv_resource_info[i].mip_levels = 0;
                dsv_resource_info[i].format = DXGI_FORMAT_D32_FLOAT;
                dsv_resource_info[i].layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
                dsv_resource_info[i].flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
                dsv_resource_info[i].current_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
                create_resource(&device_info, &dsv_resource_info[i]);
        }

        // Create depth buffer view
        struct gpu_view_info dsv_view_info;
        dsv_view_info.dsv_dimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        create_depthstencil_view(&device_info, &dsv_descriptor_info,
                 dsv_resource_info, &dsv_view_info);

        // Compile vertex shader
        struct gpu_shader_info vert_shader_info;
        vert_shader_info.shader_file = L"shaders\\tri_vert_shader.hlsl";
        vert_shader_info.shader_target = "vs_6_5";
        compile_shader(&vert_shader_info);

        // Compile pixel shader
        struct gpu_shader_info pix_shader_info;
        pix_shader_info.shader_file = L"shaders\\tri_pix_shader.hlsl";
        pix_shader_info.shader_target = "ps_6_5";
        compile_shader(&pix_shader_info);

        // Create root signature
        #define NUM_GRAPHICS_ROOT_PARAM 4
        struct gpu_root_param_info graphics_root_param_infos[NUM_GRAPHICS_ROOT_PARAM];

        // Layout of root sig
        #define PER_OBJ_CBV_GRAPHICS_ROOT_PARAM_INDEX 0
        #define PER_FRAME_CBV_GRAPHICS_ROOT_PARAM_INDEX 1
        #define SRV_GRAPHICS_ROOT_PARAM_INDEX 2
        #define SAMPLER_GRAPHICS_ROOT_PARAM_INDEX 3

        graphics_root_param_infos[PER_OBJ_CBV_GRAPHICS_ROOT_PARAM_INDEX].range_type =
                D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        graphics_root_param_infos[PER_OBJ_CBV_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors = 1;
        graphics_root_param_infos[PER_OBJ_CBV_GRAPHICS_ROOT_PARAM_INDEX].base_shader_register = 0;
        graphics_root_param_infos[PER_OBJ_CBV_GRAPHICS_ROOT_PARAM_INDEX].shader_visbility =
                D3D12_SHADER_VISIBILITY_VERTEX;

        graphics_root_param_infos[PER_FRAME_CBV_GRAPHICS_ROOT_PARAM_INDEX].range_type =
                D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        graphics_root_param_infos[PER_FRAME_CBV_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors = 1;
        graphics_root_param_infos[PER_FRAME_CBV_GRAPHICS_ROOT_PARAM_INDEX].base_shader_register = 1;
        graphics_root_param_infos[PER_FRAME_CBV_GRAPHICS_ROOT_PARAM_INDEX].shader_visbility =
                D3D12_SHADER_VISIBILITY_VERTEX;

        graphics_root_param_infos[SRV_GRAPHICS_ROOT_PARAM_INDEX].range_type =
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        graphics_root_param_infos[SRV_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors = 1;
        graphics_root_param_infos[SRV_GRAPHICS_ROOT_PARAM_INDEX].base_shader_register = 0;
        graphics_root_param_infos[SRV_GRAPHICS_ROOT_PARAM_INDEX].shader_visbility =
                D3D12_SHADER_VISIBILITY_PIXEL;

        graphics_root_param_infos[SAMPLER_GRAPHICS_ROOT_PARAM_INDEX].range_type =
                D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        graphics_root_param_infos[SAMPLER_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors = 1;
        graphics_root_param_infos[SAMPLER_GRAPHICS_ROOT_PARAM_INDEX].base_shader_register = 0;
        graphics_root_param_infos[SAMPLER_GRAPHICS_ROOT_PARAM_INDEX].shader_visbility =
                D3D12_SHADER_VISIBILITY_PIXEL;

        struct gpu_root_sig_info graphics_root_sig_info;
        create_wstring(graphics_root_sig_info.name, L"Graphics Root sig");
        create_root_sig(&device_info, graphics_root_param_infos, NUM_GRAPHICS_ROOT_PARAM,
                &graphics_root_sig_info);

        // Create graphics pipeline state object
        struct gpu_pso_info graphics_pso_info;
        create_wstring(graphics_pso_info.name, L"Graphics PSO");
        graphics_pso_info.type = PSO_TYPE_GRAPHICS;
        graphics_pso_info.graphics_pso_info.vert_shader_byte_code =
                vert_shader_info.shader_byte_code;
        graphics_pso_info.graphics_pso_info.vert_shader_byte_code_len =
                vert_shader_info.shader_byte_code_len;
        graphics_pso_info.graphics_pso_info.pix_shader_byte_code =
                pix_shader_info.shader_byte_code;
        graphics_pso_info.graphics_pso_info.pix_shader_byte_code_len =
                pix_shader_info.shader_byte_code_len;
        graphics_pso_info.graphics_pso_info.dom_shader_byte_code = NULL;
        graphics_pso_info.graphics_pso_info.dom_shader_byte_code_len = 0;
        graphics_pso_info.graphics_pso_info.hull_shader_byte_code = NULL;
        graphics_pso_info.graphics_pso_info.hull_shader_byte_code_len = 0;
        graphics_pso_info.graphics_pso_info.geom_shader_byte_code = NULL;
        graphics_pso_info.graphics_pso_info.geom_shader_byte_code_len = 0;
        graphics_pso_info.graphics_pso_info.render_target_format =
                tmp_rtv_resource_info[swp_chain_info.current_buffer_index].format;
        graphics_pso_info.graphics_pso_info.depth_target_format =
                dsv_resource_info[swp_chain_info.current_buffer_index].format;
        create_pso(&device_info, &vert_input_info[0], &graphics_root_sig_info,
                &graphics_pso_info);

        // Now we need to create a view port for the screen we are going to be drawing to
        struct gpu_viewport_info viewport_info;
        viewport_info.width = (float) wnd_info.width;
        viewport_info.height = (float) wnd_info.height;
        create_viewport(&viewport_info);

        // Create the scissor rectangle for the pixel operations
        struct gpu_scissor_rect_info scissor_rect_info;
        create_scissor_rect(&scissor_rect_info);

        // Calculate project view matrix
        struct camera_info cam_info;
        calc_pv_mat(&cam_info);

        // Create sampler descriptor
        // Do I need two samplers, one for each frame? I don't know.
        struct gpu_descriptor_info sampler_descriptor_info;
        create_wstring(sampler_descriptor_info.name, L"Sampler heap");
        sampler_descriptor_info.type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        sampler_descriptor_info.num_descriptors =
                graphics_root_param_infos[SAMPLER_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors;
        sampler_descriptor_info.flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        create_descriptor(&device_info, &sampler_descriptor_info);

        // Create sampler
        create_sampler(&device_info, &sampler_descriptor_info);

        // Create constant buffer and texture resource descriptor
        UINT num_graphics_cbv_srv_uav_descriptors =
                (graphics_root_param_infos[PER_OBJ_CBV_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors * (UINT) gltf_mesh_count) +
                graphics_root_param_infos[PER_FRAME_CBV_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors +
                graphics_root_param_infos[SRV_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors;
        struct gpu_descriptor_info graphics_cbv_srv_uav_descriptor_info;
        create_wstring(graphics_cbv_srv_uav_descriptor_info.name,
                L"Graphics CBV SRV UAV heap");
        graphics_cbv_srv_uav_descriptor_info.type =
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        graphics_cbv_srv_uav_descriptor_info.num_descriptors =
                swp_chain_info.buffer_count * num_graphics_cbv_srv_uav_descriptors;
        graphics_cbv_srv_uav_descriptor_info.flags =
                D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        create_descriptor(&device_info, &graphics_cbv_srv_uav_descriptor_info);

        // This is bad I know.
        UINT PER_OBJ_CBV_GRAPHICS_DESCRIPTOR_INDEX = 0;
        UINT PER_FRAME_CBV_GRAPHICS_DESCRIPTOR_INDEX = (UINT) gltf_mesh_count;
        UINT SRV_GRAPHICS_DESCRIPTOR_INDEX = (UINT) gltf_mesh_count +
                graphics_root_param_infos[PER_FRAME_CBV_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors;

        // Create per obj constant buffer resource
        // NEED ONE PER MESH!!
        struct gpu_resource_info *per_obj_graphics_cbv_resource_info;
        UINT per_obj_descriptor_count = graphics_root_param_infos[PER_OBJ_CBV_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors *
                (UINT) gltf_mesh_count;
        per_obj_graphics_cbv_resource_info = malloc(swp_chain_info.buffer_count *
                per_obj_descriptor_count * sizeof (struct gpu_resource_info));
        for (UINT i = 0; i < swp_chain_info.buffer_count; ++i) {
                for (UINT j = 0;
                        j < graphics_root_param_infos[PER_OBJ_CBV_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors *
                        gltf_mesh_count;
                        ++j) {

                        UINT index = per_obj_descriptor_count * i + j;
                        create_wstring(per_obj_graphics_cbv_resource_info[index].name,
                                L"Per object Graphics CBV resource %d", index);
                        per_obj_graphics_cbv_resource_info[index].type =
                                D3D12_HEAP_TYPE_UPLOAD;
                        per_obj_graphics_cbv_resource_info[index].dimension =
                                D3D12_RESOURCE_DIMENSION_BUFFER;
                        per_obj_graphics_cbv_resource_info[index].width =
                                align_offset(sizeof (node_info[0].world_transform),
                                256);
                        per_obj_graphics_cbv_resource_info[index].height = 1;
                        per_obj_graphics_cbv_resource_info[index].mip_levels = 1;
                        per_obj_graphics_cbv_resource_info[index].format =
                                DXGI_FORMAT_UNKNOWN;
                        per_obj_graphics_cbv_resource_info[index].layout =
                                D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                        per_obj_graphics_cbv_resource_info[index].flags =
                                D3D12_RESOURCE_FLAG_NONE;
                        per_obj_graphics_cbv_resource_info[index].current_state =
                                D3D12_RESOURCE_STATE_GENERIC_READ;
                        create_resource(&device_info,
                                &per_obj_graphics_cbv_resource_info[index]);

                        update_cpu_handle(&graphics_cbv_srv_uav_descriptor_info,
                                num_graphics_cbv_srv_uav_descriptors * i +
                                PER_OBJ_CBV_GRAPHICS_DESCRIPTOR_INDEX + j);

                        // Create constant buffer view
                        create_constant_buffer_view(&device_info,
                                &graphics_cbv_srv_uav_descriptor_info,
                                &per_obj_graphics_cbv_resource_info[index]);
                }
        }

        // Create per frame constant buffer resource
        struct gpu_resource_info *per_frame_graphics_cbv_resource_info;
        per_frame_graphics_cbv_resource_info = malloc(swp_chain_info.buffer_count *
                graphics_root_param_infos[PER_FRAME_CBV_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors *
                sizeof (struct gpu_resource_info));
        for (UINT i = 0; i < swp_chain_info.buffer_count *
                graphics_root_param_infos[PER_FRAME_CBV_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors;
                ++i) {

                create_wstring(per_frame_graphics_cbv_resource_info[i].name,
                        L"Per frame Graphics CBV resource %d", i);
                per_frame_graphics_cbv_resource_info[i].type =
                        D3D12_HEAP_TYPE_UPLOAD;
                per_frame_graphics_cbv_resource_info[i].dimension =
                        D3D12_RESOURCE_DIMENSION_BUFFER;
                per_frame_graphics_cbv_resource_info[i].width =
                        align_offset(sizeof (cam_info.pv_mat), 256);
                per_frame_graphics_cbv_resource_info[i].height = 1;
                per_frame_graphics_cbv_resource_info[i].mip_levels = 1;
                per_frame_graphics_cbv_resource_info[i].format =
                        DXGI_FORMAT_UNKNOWN;
                per_frame_graphics_cbv_resource_info[i].layout =
                        D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                per_frame_graphics_cbv_resource_info[i].flags =
                        D3D12_RESOURCE_FLAG_NONE;
                per_frame_graphics_cbv_resource_info[i].current_state =
                        D3D12_RESOURCE_STATE_GENERIC_READ;
                create_resource(&device_info,
                        &per_frame_graphics_cbv_resource_info[i]);

                // Upload constant buffer resource
                upload_resources(&per_frame_graphics_cbv_resource_info[i],
                        cam_info.pv_mat);

                update_cpu_handle(&graphics_cbv_srv_uav_descriptor_info,
                        num_graphics_cbv_srv_uav_descriptors * i +
                        PER_FRAME_CBV_GRAPHICS_DESCRIPTOR_INDEX);

                // Create constant buffer view
                create_constant_buffer_view(&device_info,
                        &graphics_cbv_srv_uav_descriptor_info,
                        &per_frame_graphics_cbv_resource_info[i]);
        }

        // Get checker board texture material
        struct material_info checkerboard_mat_info;
        get_checkerboard_tex(256, 256, &checkerboard_mat_info);

        // Create texture upload resource
        struct gpu_resource_info tex_upload_resource_info;
        create_wstring(tex_upload_resource_info.name, L"Tex upload resource");
        tex_upload_resource_info.type = D3D12_HEAP_TYPE_UPLOAD;
        tex_upload_resource_info.dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        tex_upload_resource_info.width = checkerboard_mat_info.tex_size;
        tex_upload_resource_info.height = 1;
        tex_upload_resource_info.mip_levels = 1;
        tex_upload_resource_info.format = DXGI_FORMAT_UNKNOWN;
        tex_upload_resource_info.layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        tex_upload_resource_info.flags = D3D12_RESOURCE_FLAG_NONE;
        tex_upload_resource_info.current_state =
                D3D12_RESOURCE_STATE_GENERIC_READ;
        create_resource(&device_info, &tex_upload_resource_info);

        // Upload texture resource
        upload_resources(&tex_upload_resource_info, checkerboard_mat_info.tex);

        // Create texture resource
        struct gpu_resource_info *tex_resource_info;
        tex_resource_info = malloc(swp_chain_info.buffer_count *
                graphics_root_param_infos[SRV_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors *
                sizeof (struct gpu_resource_info));

        for (UINT i = 0; i < swp_chain_info.buffer_count *
                graphics_root_param_infos[SRV_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors;
                ++i) {
                create_wstring(tex_resource_info[i].name,
                        L"Tex resource %d", i);
                tex_resource_info[i].type = D3D12_HEAP_TYPE_DEFAULT;
                tex_resource_info[i].dimension =
                        D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                tex_resource_info[i].width = 256;
                tex_resource_info[i].height = 256;
                tex_resource_info[i].mip_levels = 1;
                tex_resource_info[i].format = DXGI_FORMAT_R8G8B8A8_UNORM;
                tex_resource_info[i].layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
                tex_resource_info[i].flags = D3D12_RESOURCE_FLAG_NONE;
                tex_resource_info[i].current_state =
                        D3D12_RESOURCE_STATE_COMMON;
                create_resource(&device_info, &tex_resource_info[i]);

                update_cpu_handle(&graphics_cbv_srv_uav_descriptor_info,
                        num_graphics_cbv_srv_uav_descriptors * i +
                        SRV_GRAPHICS_DESCRIPTOR_INDEX);

                // Create shader resource view
                struct gpu_view_info tex_view_info;
                tex_view_info.srv_dimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                create_shader_resource_view(&device_info, 
                        &graphics_cbv_srv_uav_descriptor_info,
                        &tex_resource_info[i], &tex_view_info);

                rec_copy_texture_region_cmd(&copy_cmd_list_info,
                        &tex_resource_info[i], &tex_upload_resource_info);
        }

        // Close command list for execution
        close_cmd_list(&copy_cmd_list_info);

        // Exexute command list
        execute_cmd_list(&copy_queue_info, &copy_cmd_list_info);

        // Signal GPU for texture upload of copy queue work
        signal_gpu_with_fence(&copy_queue_info, &fence_info,
                swp_chain_info.current_buffer_index);

        // Make sure render queue which will use the texture waits for it be uploaded
        wait_for_fence_on_gpu(&render_queue_info, &fence_info,
                swp_chain_info.current_buffer_index);

        #define MAX_RESOURCE_TRANSITION_COUNT 100
        struct gpu_resource_info *transition_resource_info_list[MAX_RESOURCE_TRANSITION_COUNT];
        D3D12_RESOURCE_STATES transition_resource_states_list[MAX_RESOURCE_TRANSITION_COUNT];

        UINT resource_transition_index = 0;
        for (UINT i = 0; i < swp_chain_info.buffer_count *
                graphics_root_param_infos[SRV_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors;
                ++i, ++resource_transition_index) {

                transition_resource_info_list[resource_transition_index] =
                        &tex_resource_info[i];
                transition_resource_states_list[resource_transition_index] =
                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        }

        for (UINT i = 0; i < gltf_mesh_count; ++i) {

                transition_resource_info_list[resource_transition_index] =
                        &vert_gpu_resource_info[i];
                transition_resource_states_list[resource_transition_index++] =
                        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

                transition_resource_info_list[resource_transition_index] =
                        &indices_gpu_resource_info[i];
                transition_resource_states_list[resource_transition_index++] =
                        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        }

        transition_resources(&render_cmd_list_info,
                transition_resource_info_list, transition_resource_states_list,
                resource_transition_index);

        // Compile compute shader
        struct gpu_shader_info comp_shader_info;
        comp_shader_info.shader_file = L"shaders\\tri_comp_shader.hlsl";
        comp_shader_info.shader_target = "cs_6_5";
        compile_shader(&comp_shader_info);

        // Create compute root signature
        #define NUM_COMPUTE_ROOT_PARAM 3
        struct gpu_root_param_info compute_root_param_infos[NUM_COMPUTE_ROOT_PARAM];

        // Layout of compute root sig
        #define CBV_COMPUTE_ROOT_PARAM_INDEX 0
        #define SRV_COMPUTE_ROOT_PARAM_INDEX 1
        #define UAV_COMPUTE_ROOT_PARAM_INDEX 2

        compute_root_param_infos[CBV_COMPUTE_ROOT_PARAM_INDEX].range_type =
                D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        compute_root_param_infos[CBV_COMPUTE_ROOT_PARAM_INDEX].num_descriptors = 1;
        compute_root_param_infos[CBV_COMPUTE_ROOT_PARAM_INDEX].base_shader_register = 0;
        compute_root_param_infos[CBV_COMPUTE_ROOT_PARAM_INDEX].shader_visbility =
                D3D12_SHADER_VISIBILITY_ALL;

        compute_root_param_infos[SRV_COMPUTE_ROOT_PARAM_INDEX].range_type =
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        compute_root_param_infos[SRV_COMPUTE_ROOT_PARAM_INDEX].num_descriptors = 1;
        compute_root_param_infos[SRV_COMPUTE_ROOT_PARAM_INDEX].base_shader_register = 0;
        compute_root_param_infos[SRV_COMPUTE_ROOT_PARAM_INDEX].shader_visbility =
                D3D12_SHADER_VISIBILITY_ALL;

        compute_root_param_infos[UAV_COMPUTE_ROOT_PARAM_INDEX].range_type =
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        compute_root_param_infos[UAV_COMPUTE_ROOT_PARAM_INDEX].num_descriptors = 1;
        compute_root_param_infos[UAV_COMPUTE_ROOT_PARAM_INDEX].base_shader_register = 0;
        compute_root_param_infos[UAV_COMPUTE_ROOT_PARAM_INDEX].shader_visbility =
                D3D12_SHADER_VISIBILITY_ALL;

        struct gpu_root_sig_info compute_root_sig_info;
        create_wstring(compute_root_sig_info.name, L"Compute Root sig");
        create_root_sig(&device_info, compute_root_param_infos,
                NUM_COMPUTE_ROOT_PARAM, &compute_root_sig_info);

        // Create compute pipeline state object
        struct gpu_pso_info compute_pso_info;
        create_wstring(compute_pso_info.name, L"Compute PSO");
        compute_pso_info.type = PSO_TYPE_COMPUTE;
        compute_pso_info.compute_pso_info.comp_shader_byte_code =
                comp_shader_info.shader_byte_code;
        compute_pso_info.compute_pso_info.comp_shader_byte_code_len =
                comp_shader_info.shader_byte_code_len;
        create_pso(&device_info, NULL, &compute_root_sig_info,
                &compute_pso_info);

        // Create constant buffer descriptor for compute
        UINT num_compute_cbv_srv_uav_descriptors =
                compute_root_param_infos[CBV_COMPUTE_ROOT_PARAM_INDEX].num_descriptors +
                compute_root_param_infos[SRV_COMPUTE_ROOT_PARAM_INDEX].num_descriptors +
                compute_root_param_infos[UAV_COMPUTE_ROOT_PARAM_INDEX].num_descriptors;
        struct gpu_descriptor_info compute_cbv_srv_uav_descriptor_info;
        create_wstring(compute_cbv_srv_uav_descriptor_info.name,
                L"Compute CBV SRV UAV heap");
        compute_cbv_srv_uav_descriptor_info.type =
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        compute_cbv_srv_uav_descriptor_info.num_descriptors =
                swp_chain_info.buffer_count *
                num_compute_cbv_srv_uav_descriptors;
        compute_cbv_srv_uav_descriptor_info.flags =
                D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        create_descriptor(&device_info, &compute_cbv_srv_uav_descriptor_info);

        vec2 window_bounds = { (float) wnd_info.width,
                (float) wnd_info.height };

        #define CBV_COMPUTE_DESCRIPTOR_INDEX 0
        #define SRV_COMPUTE_DESCRIPTOR_INDEX 1
        #define UAV_COMPUTE_DESCRIPTOR_INDEX 2

        // Create compute cbv resource
        struct gpu_resource_info *compute_cbv_resource_info;
        compute_cbv_resource_info = malloc(swp_chain_info.buffer_count *
                compute_root_param_infos[CBV_COMPUTE_ROOT_PARAM_INDEX].num_descriptors *
                sizeof (struct gpu_resource_info));
        for (UINT i = 0; i < swp_chain_info.buffer_count *
                compute_root_param_infos[0].num_descriptors; ++i) {
                create_wstring(compute_cbv_resource_info[i].name,
                        L"Compute CBV resource %d", i);
                compute_cbv_resource_info[i].type = D3D12_HEAP_TYPE_UPLOAD;
                compute_cbv_resource_info[i].dimension =
                        D3D12_RESOURCE_DIMENSION_BUFFER;
                compute_cbv_resource_info[i].width =
                        align_offset(sizeof (window_bounds), 256);
                compute_cbv_resource_info[i].height = 1;
                compute_cbv_resource_info[i].mip_levels = 1;
                compute_cbv_resource_info[i].format = DXGI_FORMAT_UNKNOWN;
                compute_cbv_resource_info[i].layout =
                        D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                compute_cbv_resource_info[i].flags = D3D12_RESOURCE_FLAG_NONE;
                compute_cbv_resource_info[i].current_state =
                        D3D12_RESOURCE_STATE_GENERIC_READ;
                create_resource(&device_info, &compute_cbv_resource_info[i]);

                update_cpu_handle(&compute_cbv_srv_uav_descriptor_info,
                        num_compute_cbv_srv_uav_descriptors * i +
                        CBV_COMPUTE_DESCRIPTOR_INDEX); // 0, 3

                // Create constant buffer view
                create_constant_buffer_view(&device_info,
                        &compute_cbv_srv_uav_descriptor_info,
                        &compute_cbv_resource_info[i]);
        }

        // Create BLAS RTX acceleration struct.
        // This takes in all the mesh data
        struct gpu_resource_info *blas_dest_resource_info;
        struct gpu_resource_info *blas_scratch_resource_info;

        blas_dest_resource_info = malloc(gltf_mesh_count *
                sizeof (struct gpu_resource_info));
        blas_scratch_resource_info = malloc(gltf_mesh_count *
                sizeof (struct gpu_resource_info));

        struct gpu_dxr_info *blas_info;
        blas_info = malloc(gltf_mesh_count * sizeof (struct gpu_dxr_info));

        struct gpu_resource_info *uav_barrier_resource_info_list[MAX_RESOURCE_TRANSITION_COUNT];

        resource_transition_index = 0;
        UINT uav_barrier_index = 0;
        for (UINT i = 0; i < gltf_mesh_count; ++i) {

                struct gltf_mesh_info *mesh_info = node_info[i].mesh_info;

                struct gltf_primitive_info *primitive_info;
                primitive_info = &mesh_info->primitive_info;

                create_blas_prebuild_info(&device_info, primitive_info,
                &vert_gpu_resource_info[i], &indices_gpu_resource_info[i],
                &blas_info[i]);

                create_wstring(blas_dest_resource_info[i].name,
                        L"BLAS Dest resource %d", i);
                blas_dest_resource_info[i].type = D3D12_HEAP_TYPE_DEFAULT;
                blas_dest_resource_info[i].dimension =
                        D3D12_RESOURCE_DIMENSION_BUFFER;
                blas_dest_resource_info[i].width =
                        blas_info[i].prebuild_info.ResultDataMaxSizeInBytes;
                blas_dest_resource_info[i].height = 1;
                blas_dest_resource_info[i].mip_levels = 1;
                blas_dest_resource_info[i].format = DXGI_FORMAT_UNKNOWN;
                blas_dest_resource_info[i].layout =
                        D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                blas_dest_resource_info[i].flags =
                        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                blas_dest_resource_info[i].current_state =
                        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
                create_resource(&device_info, &blas_dest_resource_info[i]);

                create_wstring(blas_scratch_resource_info[i].name,
                        L"BLAS Scratch resource %d", i);
                blas_scratch_resource_info[i].type = D3D12_HEAP_TYPE_DEFAULT;
                blas_scratch_resource_info[i].dimension =
                        D3D12_RESOURCE_DIMENSION_BUFFER;
                blas_scratch_resource_info[i].width =
                        blas_info[i].prebuild_info.ScratchDataSizeInBytes;
                blas_scratch_resource_info[i].height = 1;
                blas_scratch_resource_info[i].mip_levels = 1;
                blas_scratch_resource_info[i].format = DXGI_FORMAT_UNKNOWN;
                blas_scratch_resource_info[i].layout =
                        D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                blas_scratch_resource_info[i].flags =
                        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                blas_scratch_resource_info[i].current_state =
                        D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                create_resource(&device_info, &blas_scratch_resource_info[i]);

                rec_build_dxr_acceleration_struct(&render_cmd_list_info,
                        &blas_dest_resource_info[i],
                        &blas_scratch_resource_info[i], NULL,
                        &blas_info[i]);

                transition_resource_info_list[resource_transition_index] =
                        &vert_gpu_resource_info[i];
                transition_resource_info_list[resource_transition_index + 1] =
                        &indices_gpu_resource_info[i];

                transition_resource_states_list[resource_transition_index] =
                        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
                transition_resource_states_list[resource_transition_index + 1] =
                        D3D12_RESOURCE_STATE_INDEX_BUFFER;

                resource_transition_index += 2;

                uav_barrier_resource_info_list[uav_barrier_index++] = &blas_dest_resource_info[i];
        }

        transition_resources(&render_cmd_list_info, transition_resource_info_list,
                        transition_resource_states_list, resource_transition_index);

        uav_barrier(&render_cmd_list_info,
                uav_barrier_resource_info_list, uav_barrier_index);

        // Should be able to hold instance desc for every mesh
        struct gpu_resource_info instance_resource_info;
        create_wstring(instance_resource_info.name, L"TLAS Instance resource");
        instance_resource_info.type = D3D12_HEAP_TYPE_UPLOAD;
        instance_resource_info.dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        instance_resource_info.width =
                align_offset(gltf_mesh_count *
                sizeof (D3D12_RAYTRACING_INSTANCE_DESC), 16);
        instance_resource_info.height = 1;
        instance_resource_info.mip_levels = 1;
        instance_resource_info.format = DXGI_FORMAT_UNKNOWN;
        instance_resource_info.layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        instance_resource_info.flags = D3D12_RESOURCE_FLAG_NONE;
        instance_resource_info.current_state =
                D3D12_RESOURCE_STATE_GENERIC_READ;
        create_resource(&device_info, &instance_resource_info);

        struct gpu_dxr_info tlas_info;
        create_tlas_prebuild_info(&device_info, gltf_mesh_count, &node_info[0],
                &blas_dest_resource_info[0], &instance_resource_info,
                &tlas_info);

        struct gpu_resource_info *tlas_dest_resource_info;
        tlas_dest_resource_info = malloc(swp_chain_info.buffer_count *
                compute_root_param_infos[SRV_COMPUTE_ROOT_PARAM_INDEX].num_descriptors *
                sizeof (struct gpu_resource_info));
        for (UINT i = 0; i < swp_chain_info.buffer_count *
                compute_root_param_infos[SRV_COMPUTE_ROOT_PARAM_INDEX].num_descriptors;
                ++i) {
                create_wstring(tlas_dest_resource_info[i].name,
                        L"TLAS Dest resource %d", i);
                tlas_dest_resource_info[i].type = D3D12_HEAP_TYPE_DEFAULT;
                tlas_dest_resource_info[i].dimension =
                        D3D12_RESOURCE_DIMENSION_BUFFER;
                tlas_dest_resource_info[i].width =
                        tlas_info.prebuild_info.ResultDataMaxSizeInBytes;
                tlas_dest_resource_info[i].height = 1;
                tlas_dest_resource_info[i].mip_levels = 1;
                tlas_dest_resource_info[i].format = DXGI_FORMAT_UNKNOWN;
                tlas_dest_resource_info[i].layout =
                        D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                tlas_dest_resource_info[i].flags =
                        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                tlas_dest_resource_info[i].current_state =
                        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
                create_resource(&device_info, &tlas_dest_resource_info[i]);
        }

        struct gpu_resource_info *tlas_scratch_resource_info;
        tlas_scratch_resource_info = malloc(swp_chain_info.buffer_count *
                compute_root_param_infos[SRV_COMPUTE_ROOT_PARAM_INDEX].num_descriptors *
                sizeof (struct gpu_resource_info));
        for (UINT i = 0; i < swp_chain_info.buffer_count *
                compute_root_param_infos[SRV_COMPUTE_ROOT_PARAM_INDEX].num_descriptors; ++i) {
                create_wstring(tlas_scratch_resource_info[i].name,
                        L"TLAS Scratch resource %d", i);
                tlas_scratch_resource_info[i].type = D3D12_HEAP_TYPE_DEFAULT;
                tlas_scratch_resource_info[i].dimension =
                        D3D12_RESOURCE_DIMENSION_BUFFER;
                tlas_scratch_resource_info[i].width =
                        tlas_info.prebuild_info.ScratchDataSizeInBytes;
                tlas_scratch_resource_info[i].height = 1;
                tlas_scratch_resource_info[i].mip_levels = 1;
                tlas_scratch_resource_info[i].format = DXGI_FORMAT_UNKNOWN;
                tlas_scratch_resource_info[i].layout =
                        D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                tlas_scratch_resource_info[i].flags =
                        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                tlas_scratch_resource_info[i].current_state =
                        D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                create_resource(&device_info, &tlas_scratch_resource_info[i]);
        }

        for (UINT i = 0; i < swp_chain_info.buffer_count *
                compute_root_param_infos[SRV_COMPUTE_ROOT_PARAM_INDEX].num_descriptors;
                ++i) {

                rec_build_dxr_acceleration_struct(&render_cmd_list_info,
                        &tlas_dest_resource_info[i],
                        &tlas_scratch_resource_info[i], &instance_resource_info,
                        &tlas_info);

                // UAV write barrier
                uav_barrier_resource_info_list[0] = &tlas_dest_resource_info[i];
                uav_barrier(&render_cmd_list_info,
                        uav_barrier_resource_info_list, 1);
        }

        for (UINT i = 0; i < swp_chain_info.buffer_count *
                compute_root_param_infos[SRV_COMPUTE_ROOT_PARAM_INDEX].num_descriptors;
                ++i) {
                update_cpu_handle(&compute_cbv_srv_uav_descriptor_info,
                        num_compute_cbv_srv_uav_descriptors * i + SRV_COMPUTE_DESCRIPTOR_INDEX); // 1, 4

                struct gpu_view_info rtx_acc_view_info;
                rtx_acc_view_info.srv_dimension =
                        D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
                create_shader_resource_view(&device_info,
                        &compute_cbv_srv_uav_descriptor_info,
                        &tlas_dest_resource_info[i], &rtx_acc_view_info);
        }

        for (UINT i = 0; i < swp_chain_info.buffer_count *
                compute_root_param_infos[UAV_COMPUTE_ROOT_PARAM_INDEX].num_descriptors;
                ++i) {
                update_cpu_handle(&compute_cbv_srv_uav_descriptor_info,
                        num_compute_cbv_srv_uav_descriptors * i + UAV_COMPUTE_DESCRIPTOR_INDEX); // 2, 5

                // Create unordered access view
                struct gpu_view_info tex_view_info;
                tex_view_info.uav_dimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                create_unorderd_access_view(&device_info,
                        &compute_cbv_srv_uav_descriptor_info,
                        &tmp_rtv_resource_info[i], &tex_view_info);
        }

        LONG_PTR wndproc_data[] = { (LONG_PTR) &wnd_info,
                (LONG_PTR) &device_info, (LONG_PTR) &present_queue_info,
                (LONG_PTR) &swp_chain_info, (LONG_PTR) &rtv_descriptor_info,
                (LONG_PTR) &rtv_resource_info[0],
                (LONG_PTR) &tmp_rtv_descriptor_info,
                (LONG_PTR) &tmp_rtv_resource_info[0], (LONG_PTR) &fence_info,
                (LONG_PTR) &dsv_descriptor_info, (LONG_PTR) &dsv_resource_info[0],
                (LONG_PTR) &compute_root_param_infos[0],
                (LONG_PTR) &compute_cbv_srv_uav_descriptor_info,
                (LONG_PTR) &num_compute_cbv_srv_uav_descriptors
        };
        SetWindowLongPtr(wnd_info.hwnd, GWLP_USERDATA, (LONG_PTR) wndproc_data);

        UINT queued_window_msg = WM_NULL;
        do {
                queued_window_msg = window_message_loop();

                update_cpu_handle(&tmp_rtv_descriptor_info,
                        swp_chain_info.current_buffer_index);

                update_cpu_handle(&dsv_descriptor_info,
                        swp_chain_info.current_buffer_index);

                // Set the render target and depth target
                rec_set_render_target_cmd(&render_cmd_list_info,
                        &tmp_rtv_descriptor_info, &dsv_descriptor_info);

                // Clear render target
                float clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
                rec_clear_rtv_cmd(&render_cmd_list_info,
                        &tmp_rtv_descriptor_info, clear_color);

                // Clear depth target
                rec_clear_dsv_cmd(&render_cmd_list_info,
                        &dsv_descriptor_info);

                // Set pipeline state
                rec_set_pipeline_state_cmd(&render_cmd_list_info,
                        &graphics_pso_info);

                // Set viewport
                rec_set_viewport_cmd(&render_cmd_list_info, &viewport_info);

                // Set scissor rect
                rec_set_scissor_rect_cmd(&render_cmd_list_info,
                        &scissor_rect_info);

                // Set primitve
                rec_set_primitive_cmd(&render_cmd_list_info,
                        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                // Set root signature
                rec_set_graphics_root_sig_cmd(&render_cmd_list_info,
                        &graphics_root_sig_info);

                // Set descriptor heap for sampler
                rec_set_descriptor_heap_cmd(&render_cmd_list_info,
                        &sampler_descriptor_info);

                // Set sampler table
                rec_set_graphics_root_descriptor_table_cmd(
                        &render_cmd_list_info, SAMPLER_GRAPHICS_ROOT_PARAM_INDEX,
                        &sampler_descriptor_info);

                // Set descriptor heap for constant buffer and texture resource
                rec_set_descriptor_heap_cmd(&render_cmd_list_info,
                        &graphics_cbv_srv_uav_descriptor_info);

                // Update gpu handle to cb's position in descriptor heap
                update_gpu_handle(&graphics_cbv_srv_uav_descriptor_info,
                        num_graphics_cbv_srv_uav_descriptors *
                        swp_chain_info.current_buffer_index +
                        PER_FRAME_CBV_GRAPHICS_DESCRIPTOR_INDEX);

                // Set constant buffer table
                rec_set_graphics_root_descriptor_table_cmd(
                        &render_cmd_list_info, PER_FRAME_CBV_GRAPHICS_ROOT_PARAM_INDEX,
                        &graphics_cbv_srv_uav_descriptor_info);

                // Update gpu handle to textures srv position in descriptor heap
                update_gpu_handle(&graphics_cbv_srv_uav_descriptor_info,
                        num_graphics_cbv_srv_uav_descriptors *
                        swp_chain_info.current_buffer_index +
                        SRV_GRAPHICS_DESCRIPTOR_INDEX);

                // Set shader resource table
                rec_set_graphics_root_descriptor_table_cmd(
                        &render_cmd_list_info, SRV_GRAPHICS_ROOT_PARAM_INDEX,
                        &graphics_cbv_srv_uav_descriptor_info);

                for (UINT i = 0; i < gltf_mesh_count; ++i) {

                        // Update gpu handle to cb's position in descriptor heap
                        update_gpu_handle(&graphics_cbv_srv_uav_descriptor_info,
                        num_graphics_cbv_srv_uav_descriptors *
                        swp_chain_info.current_buffer_index +
                        PER_OBJ_CBV_GRAPHICS_DESCRIPTOR_INDEX + i);

                        // Set constant buffer table
                        rec_set_graphics_root_descriptor_table_cmd(
                        &render_cmd_list_info, PER_OBJ_CBV_GRAPHICS_ROOT_PARAM_INDEX,
                        &graphics_cbv_srv_uav_descriptor_info);

                        // Upload constant buffer resource
                        UINT index = per_obj_descriptor_count * swp_chain_info.current_buffer_index + i;
                        upload_resources(&per_obj_graphics_cbv_resource_info[index],
                        node_info[i].world_transform[0]);

                        struct gltf_mesh_info *mesh_info = node_info[i].mesh_info;

                        struct gltf_primitive_info *primitive_info;
                        primitive_info = &mesh_info->primitive_info;

                        struct gltf_attribute_info *attribute_info;
                        attribute_info = &primitive_info->attribute_info;

                        // Set vertex buffer
                        rec_set_vertex_buffer_cmd(&render_cmd_list_info,
                                &vert_gpu_resource_info[i],
                                (UINT) attribute_info->attribute_data_stride);

                        // Set index buffer
                        rec_set_index_buffer_cmd(&render_cmd_list_info,
                                &indices_gpu_resource_info[i],
                                primitive_info);

                        // Draw indexed instanced
                        rec_draw_indexed_instance_cmd(&render_cmd_list_info,
                                (UINT) primitive_info->indices_count, 1);
                }

                transition_resource_info_list[0] =
                        &tmp_rtv_resource_info[swp_chain_info.current_buffer_index];

                transition_resource_states_list[0] =
                        D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

                transition_resources(&render_cmd_list_info,
                        transition_resource_info_list,
                        transition_resource_states_list, 1);

                // Close command list for execution
                close_cmd_list(&render_cmd_list_info);

                // Exexute command list
                execute_cmd_list(&render_queue_info, &render_cmd_list_info);

                // Signal GPU for render queue's completion
                signal_gpu_with_fence(&render_queue_info, &fence_info,
                        swp_chain_info.current_buffer_index);

                // compute queue needs to wait for render queue's completion
                wait_for_fence_on_gpu(&compute_queue_info, &fence_info,
                        swp_chain_info.current_buffer_index);

                // Set pipeline state
                rec_set_pipeline_state_cmd(&compute_cmd_list_info,
                        &compute_pso_info);

                // Set root signature
                rec_set_compute_root_sig_cmd(&compute_cmd_list_info,
                        &compute_root_sig_info);

                // Set descriptor heap for compute constant buffer
                rec_set_descriptor_heap_cmd(&compute_cmd_list_info,
                        &compute_cbv_srv_uav_descriptor_info);

                window_bounds[0] = (float) wnd_info.width;
                window_bounds[1] = (float) wnd_info.height;

                // Upload constant buffer resource
                upload_resources(&compute_cbv_resource_info[swp_chain_info.current_buffer_index],
                        window_bounds);

                update_gpu_handle(&compute_cbv_srv_uav_descriptor_info,
                        num_compute_cbv_srv_uav_descriptors *
                        swp_chain_info.current_buffer_index +
                        CBV_COMPUTE_DESCRIPTOR_INDEX);

                // Set cbv
                rec_set_compute_root_descriptor_table_cmd(
                        &compute_cmd_list_info, CBV_COMPUTE_ROOT_PARAM_INDEX,
                        &compute_cbv_srv_uav_descriptor_info);

                update_gpu_handle(&compute_cbv_srv_uav_descriptor_info,
                        num_compute_cbv_srv_uav_descriptors *
                        swp_chain_info.current_buffer_index +
                        SRV_COMPUTE_DESCRIPTOR_INDEX);

                // Set rtx and srv table
                rec_set_compute_root_descriptor_table_cmd(
                        &compute_cmd_list_info, SRV_COMPUTE_ROOT_PARAM_INDEX,
                        &compute_cbv_srv_uav_descriptor_info);

                update_gpu_handle(&compute_cbv_srv_uav_descriptor_info,
                        num_compute_cbv_srv_uav_descriptors *
                        swp_chain_info.current_buffer_index +
                        UAV_COMPUTE_DESCRIPTOR_INDEX);

                // Set uav
                rec_set_compute_root_descriptor_table_cmd(
                        &compute_cmd_list_info, UAV_COMPUTE_ROOT_PARAM_INDEX,
                        &compute_cbv_srv_uav_descriptor_info);

                // Call compute dispatch
                rec_dispatch_cmd(&compute_cmd_list_info,
                        wnd_info.width / 8,
                        wnd_info.height / 8,
                        1);

                // Close command list for execution
                close_cmd_list(&compute_cmd_list_info);

                // Exexute command list
                execute_cmd_list(&compute_queue_info, &compute_cmd_list_info);

                // Signal GPU for compute queue's completion
                signal_gpu_with_fence(&compute_queue_info, &fence_info,
                        swp_chain_info.current_buffer_index);

                // Present queue needs to wait for render queue to be done
                wait_for_fence_on_gpu(&present_queue_info, &fence_info,
                        swp_chain_info.current_buffer_index);

                transition_resource_info_list[0] =
                        &tmp_rtv_resource_info[swp_chain_info.current_buffer_index];
                transition_resource_info_list[1] =
                        &rtv_resource_info[swp_chain_info.current_buffer_index];

                transition_resource_states_list[0] =
                        D3D12_RESOURCE_STATE_COPY_SOURCE;
                transition_resource_states_list[1] =
                        D3D12_RESOURCE_STATE_COPY_DEST;

                transition_resources(&present_cmd_list_info,
                        transition_resource_info_list,
                        transition_resource_states_list, 2);

                // Point render target view cpu handle to correct descriptor in descriptor heap
                update_cpu_handle(&rtv_descriptor_info,
                        swp_chain_info.current_buffer_index);

                rec_copy_resource_cmd(&present_cmd_list_info,
                        &rtv_resource_info[swp_chain_info.current_buffer_index],
                        &tmp_rtv_resource_info[swp_chain_info.current_buffer_index]);

                transition_resource_info_list[0] =
                        &tmp_rtv_resource_info[swp_chain_info.current_buffer_index];
                transition_resource_info_list[1] =
                        &rtv_resource_info[swp_chain_info.current_buffer_index];

                transition_resource_states_list[0] =
                        D3D12_RESOURCE_STATE_RENDER_TARGET;
                transition_resource_states_list[1] =
                        D3D12_RESOURCE_STATE_PRESENT;

                transition_resources(&present_cmd_list_info,
                        transition_resource_info_list,
                        transition_resource_states_list, 2);

                // Close command list for execution
                close_cmd_list(&present_cmd_list_info);

                // Exexute command list
                execute_cmd_list(&present_queue_info, &present_cmd_list_info);

                // Present swapchain
                // This updates/toggles current_buffer_index
                present_swapchain(&swp_chain_info);

                // Signal GPU for present queues completion
                signal_gpu_with_fence(&present_queue_info, &fence_info,
                        swp_chain_info.current_buffer_index);

                // Wait for gpu to finish previous frame
                wait_for_fence_on_cpu(&fence_info,
                        swp_chain_info.current_buffer_index);

                // Reset command allocator
                reset_cmd_allocator(&render_cmd_allocator_info,
                        swp_chain_info.current_buffer_index);

                // Reset command list
                reset_cmd_list(&render_cmd_allocator_info,
                        &render_cmd_list_info, 
                        swp_chain_info.current_buffer_index);

                // Reset command allocator
                reset_cmd_allocator(&compute_cmd_allocator_info,
                        swp_chain_info.current_buffer_index);

                // Reset command list
                reset_cmd_list(&compute_cmd_allocator_info,
                        &compute_cmd_list_info,
                        swp_chain_info.current_buffer_index);

                // Reset command allocator
                reset_cmd_allocator(&present_cmd_allocator_info,
                        swp_chain_info.current_buffer_index);

                // Reset command list
                reset_cmd_list(&present_cmd_allocator_info,
                        &present_cmd_list_info,
                        swp_chain_info.current_buffer_index);

        } while (queued_window_msg != WM_QUIT);

        // Wait for GPU to finish up be starting the cleaning
        signal_gpu_with_fence(&present_queue_info, &fence_info,
                swp_chain_info.current_buffer_index);

        wait_for_fence_on_cpu(&fence_info, swp_chain_info.current_buffer_index);

        release_resource(&instance_resource_info);

        for (UINT i = 0; i < swp_chain_info.buffer_count *
                compute_root_param_infos[SRV_COMPUTE_ROOT_PARAM_INDEX].num_descriptors;
                ++i) {
                release_resource(&tlas_scratch_resource_info[i]);
        }

        free(tlas_scratch_resource_info);

        for (UINT i = 0; i < swp_chain_info.buffer_count *
                compute_root_param_infos[SRV_COMPUTE_ROOT_PARAM_INDEX].num_descriptors;
                ++i) {
                release_resource(&tlas_dest_resource_info[i]);
        }

        free(tlas_dest_resource_info);

        for (UINT i = 0; i < gltf_mesh_count; ++i) {

                release_resource(&blas_scratch_resource_info[i]);
                release_resource(&blas_dest_resource_info[i]);
        }

        free(blas_scratch_resource_info);
        free(blas_dest_resource_info);
        free(blas_info);

        for (UINT i = 0; i < swp_chain_info.buffer_count *
                compute_root_param_infos[CBV_COMPUTE_ROOT_PARAM_INDEX].num_descriptors;
                ++i) {
                release_resource(&compute_cbv_resource_info[i]);
        }

        free(compute_cbv_resource_info);

        release_descriptor(&compute_cbv_srv_uav_descriptor_info);

        // Release compute pipline state object
        release_pso(&compute_pso_info);

        release_root_sig(&compute_root_sig_info);

        // Release compute shader
        release_shader(&comp_shader_info);

        for (UINT i = 0; i < swp_chain_info.buffer_count *
                graphics_root_param_infos[SRV_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors;
                ++i) {

                release_resource(&tex_resource_info[i]);
        }

        free(tex_resource_info);

        release_resource(&tex_upload_resource_info);

        release_material(&checkerboard_mat_info);

        for (UINT i = 0; i < swp_chain_info.buffer_count *
                per_obj_descriptor_count; ++i) {

                release_resource(&per_obj_graphics_cbv_resource_info[i]);
        }

        free(per_obj_graphics_cbv_resource_info);

        for (UINT i = 0; i < swp_chain_info.buffer_count *
                graphics_root_param_infos[PER_FRAME_CBV_GRAPHICS_ROOT_PARAM_INDEX].num_descriptors;
                ++i) {

                release_resource(&per_frame_graphics_cbv_resource_info[i]);
        }

        free(per_frame_graphics_cbv_resource_info);

        release_descriptor(&graphics_cbv_srv_uav_descriptor_info);

        release_descriptor(&sampler_descriptor_info);

        // Release grahics pipeline state object
        release_pso(&graphics_pso_info);

        // Release root signature
        release_root_sig(&graphics_root_sig_info);

        // Release pixel shader
        release_shader(&pix_shader_info);

        // Release vertex shader
        release_shader(&vert_shader_info);

        // Release depth stencil buffer resource
        for (UINT i = 0; i < dsv_descriptor_info.num_descriptors; ++i) {
                release_resource(&dsv_resource_info[i]);
        }

        free(dsv_resource_info);

        // Release depth stencl buffer heap
        release_descriptor(&dsv_descriptor_info);


        for (UINT i = 0; i < gltf_mesh_count; ++i) {

                // Release vert input
                free_vertex_input(&vert_input_info[i]);

                // Release index buffer upload resource
                release_resource(&indices_upload_resource_info[i]);

                // Release index buffer resource
                release_resource(&indices_gpu_resource_info[i]);

                // Release vertex buffer upload resource
                release_resource(&vert_upload_resource_info[i]);

                // Release vertex buffer resource
                release_resource(&vert_gpu_resource_info[i]);
        }

        free(vert_input_info);
        free(indices_upload_resource_info);
        free(indices_gpu_resource_info);
        free(vert_upload_resource_info);
        free(vert_gpu_resource_info);

        release_gltf_nodes(gltf_node_count, node_info, gltf_pdata->nodes);

        free(node_info);

        release_gltf(gltf_pdata);

        // Release render fence
        release_fence(&fence_info);

        release_cmd_list(&present_cmd_list_info);

        release_cmd_allocators(&present_cmd_allocator_info);

        // Release compute command list
        release_cmd_list(&compute_cmd_list_info);

        // Release compute command allocator
        release_cmd_allocators(&compute_cmd_allocator_info);

        // Release copy command list
        release_cmd_list(&copy_cmd_list_info);

        // Release copy command allocator
        release_cmd_allocators(&copy_cmd_allocator_info);

        // Release render command list
        release_cmd_list(&render_cmd_list_info);

        // Release render command allocator
        release_cmd_allocators(&render_cmd_allocator_info);

        for (UINT i = 0; i < tmp_rtv_descriptor_info.num_descriptors; ++i) {
                release_resource(&tmp_rtv_resource_info[i]);
        }

        free(tmp_rtv_resource_info);

        release_descriptor(&tmp_rtv_descriptor_info);

        // Release the render target view buffers
        for (UINT i = 0; i < rtv_descriptor_info.num_descriptors; ++i) {
                release_resource(&rtv_resource_info[i]);
        }

        free(rtv_resource_info);

        // Release render target view descriptor heap
        release_descriptor(&rtv_descriptor_info);

        release_swapchain(&swp_chain_info);

        release_cmd_queue(&present_queue_info);

        release_cmd_queue(&copy_queue_info);

        release_cmd_queue(&compute_queue_info);

        release_cmd_queue(&render_queue_info);
 
        release_gpu_device(&device_info);

        destroy_window(&wnd_info, hInstance);

        return 0;
}