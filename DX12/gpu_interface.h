#ifndef GPU_INTERFACE_H
#define GPU_INTERFACE_H

#define COBJMACROS

#include <dxgi1_6.h>
#include "d3d12.h" // Including modified d3d12.h since their c interface is broken
#include <d3dcompiler.h>

struct gpu_device_info {
        ID3D12Debug *debug;
        IDXGIFactory5 *factory5;
        ID3D12Device *device;
};

void create_gpu_device(struct gpu_device_info *device_info);
void release_gpu_device(struct gpu_device_info *device_info);


struct gpu_cmd_queue_info {
        WCHAR name[1024];
        D3D12_COMMAND_LIST_TYPE type;
        ID3D12CommandQueue *cmd_queue;
};

void create_cmd_queue(struct gpu_device_info *device_info,
        struct gpu_cmd_queue_info *cmd_queue_info);
void release_cmd_queue(struct gpu_cmd_queue_info *cmd_queue_info);


struct gpu_resource_info {
        WCHAR name[1024];
        D3D12_HEAP_TYPE type;
        D3D12_RESOURCE_DIMENSION dimension;
        UINT64 width;
        UINT height;
        UINT16 mip_levels;
        DXGI_FORMAT format;
        D3D12_TEXTURE_LAYOUT layout;
        D3D12_RESOURCE_FLAGS flags;
        D3D12_RESOURCE_STATES current_state;
        ID3D12Resource *resource;
        D3D12_GPU_VIRTUAL_ADDRESS gpu_address;
};

void create_resource(struct gpu_device_info *device_info,
        struct gpu_resource_info *resource_info);
void release_resource(struct gpu_resource_info *resource_info);
void upload_resources(struct gpu_resource_info *resource_info, void *src_data);


struct gpu_descriptor_info {
        WCHAR name[1024];
        D3D12_DESCRIPTOR_HEAP_TYPE type;
        UINT num_descriptors;
        D3D12_DESCRIPTOR_HEAP_FLAGS flags;
        ID3D12DescriptorHeap *descriptor_heap;
        UINT stride;
        D3D12_CPU_DESCRIPTOR_HANDLE base_cpu_handle;
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
        D3D12_GPU_DESCRIPTOR_HANDLE base_gpu_handle;
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
};

void create_descriptor(struct gpu_device_info *device_info,
        struct gpu_descriptor_info *descriptor_info);
void release_descriptor(struct gpu_descriptor_info *descriptor_info);
void update_cpu_handle(struct gpu_descriptor_info *descriptor_info,
        UINT index);
void update_gpu_handle(struct gpu_descriptor_info *descriptor_info,
        UINT index);
void create_rendertarget_view(struct gpu_device_info *device_info,
        struct gpu_descriptor_info *descriptor_info,
        struct gpu_resource_info *resource_info);
void create_depthstencil_view(struct gpu_device_info *device_info,
        struct gpu_descriptor_info *descriptor_info,
        struct gpu_resource_info *resource_info);
void create_constant_buffer_view(struct gpu_device_info *device_info,
        struct gpu_descriptor_info *descriptor_info,
        struct gpu_resource_info *resource_info);
void create_unorderd_access_view(struct gpu_device_info *device_info,
        struct gpu_descriptor_info *descriptor_info,
        struct gpu_resource_info *resource_info);
void create_shader_resource_view(struct gpu_device_info *device_info,
        struct gpu_descriptor_info *descriptor_info,
        struct gpu_resource_info *resource_info);
void create_sampler(struct gpu_device_info *device_info,
        struct gpu_descriptor_info *descriptor_info);


struct gpu_cmd_allocator_info {
        WCHAR name[1024];
        D3D12_COMMAND_LIST_TYPE cmd_list_type;
        UINT cmd_allocator_count;
        ID3D12CommandAllocator **cmd_allocators;
};

void create_cmd_allocators(struct gpu_device_info *device_info,
        struct gpu_cmd_allocator_info *cmd_allocator_info);
void release_cmd_allocators(struct gpu_cmd_allocator_info *cmd_allocator_info);
void reset_cmd_allocators(struct gpu_cmd_allocator_info *cmd_allocator_info);
void reset_cmd_allocator(struct gpu_cmd_allocator_info *cmd_allocator_info, 
        UINT index);


struct gpu_cmd_list_info {
        WCHAR name[1024];
        D3D12_COMMAND_LIST_TYPE cmd_list_type;
        ID3D12GraphicsCommandList *cmd_list;
};

void create_cmd_list(struct gpu_device_info *device_info,
        struct gpu_cmd_allocator_info *cmd_allocator_info,
        struct gpu_cmd_list_info *cmd_list_info);
void release_cmd_list(struct gpu_cmd_list_info *cmd_list_info);
void close_cmd_list(struct gpu_cmd_list_info *cmd_list_info);
void execute_cmd_list(struct gpu_cmd_queue_info *cmd_queue_info,
        struct gpu_cmd_list_info *cmd_list_info);
void reset_cmd_list(struct gpu_cmd_allocator_info *cmd_allocator_info,
        struct gpu_cmd_list_info *cmd_list_info, UINT index);
void rec_copy_buffer_region_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_resource_info *dst_resource_info,
        struct gpu_resource_info *src_resource_info);
void rec_copy_texture_region_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_resource_info *dst_resource_info,
        struct gpu_resource_info *src_resource_info);
void rec_copy_resource_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_resource_info *dst_resource_info,
        struct gpu_resource_info *src_resource_info);
void rec_clear_rtv_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_descriptor_info *rtv_desc_info, float *clear_colour);
void rec_clear_dsv_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_descriptor_info *dsv_desc_info);
void rec_set_pipeline_state_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_pso_info *pso_info);
void rec_set_render_target_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_descriptor_info *rtv_desc_info,
        struct gpu_descriptor_info *dsv_desc_info);
void rec_set_viewport_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_viewport_info *viewport_info);
void rec_set_scissor_rect_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_scissor_rect_info *scissor_rect_info);
void rec_set_primitive_cmd(struct gpu_cmd_list_info *cmd_list_info,
        D3D_PRIMITIVE_TOPOLOGY prim_type);
void rec_set_compute_root_sig_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_root_sig_info *root_sig_info);
void rec_set_graphics_root_sig_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_root_sig_info *root_sig_info);
void rec_set_descriptor_heap_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_descriptor_info *descriptor_info);
void rec_set_compute_root_descriptor_table_cmd(
        struct gpu_cmd_list_info *cmd_list_info, UINT root_param_index,
        struct gpu_descriptor_info *descriptor_info);
void rec_set_graphics_root_descriptor_table_cmd(
        struct gpu_cmd_list_info *cmd_list_info, UINT root_param_index,
        struct gpu_descriptor_info *descriptor_info);
void rec_set_vertex_buffer_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_resource_info *vert_buffer, UINT stride);
void rec_set_index_buffer_cmd(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_resource_info *index_buffer);
void rec_dispatch_cmd(struct gpu_cmd_list_info *cmd_list_info,
        UINT thread_group_coun_x, UINT thread_group_coun_y,
        UINT thread_group_coun_z);
void rec_draw_indexed_instance_cmd(struct gpu_cmd_list_info *cmd_list_info,
        UINT index_count, UINT instance_count);
void transition_resources(struct gpu_cmd_list_info *cmd_list_info,
        struct gpu_resource_info **resource_info_list,
        D3D12_RESOURCE_STATES *resource_end_state_list, UINT resource_count);


struct gpu_fence_info {
        WCHAR name[1024];
        UINT num_fence_value;
        UINT64 *fence_values;
        UINT64 cur_fence_value;
        ID3D12Fence *fence;
        HANDLE fence_event;
};

void create_fence(struct gpu_device_info *device_info,
        struct gpu_fence_info *fence_info);
void release_fence(struct gpu_fence_info *fence_info);
void signal_gpu(struct gpu_cmd_queue_info *cmd_queue_info,
        struct gpu_fence_info *fence_info, UINT index);
void wait_for_fence(struct gpu_cmd_queue_info *cmd_queue_info,
        struct gpu_fence_info *fence_info, UINT index);
void wait_for_gpu(struct gpu_fence_info *fence_info, UINT index);


struct gpu_shader_info {
        LPCWSTR shader_file;
        UINT flags;
        LPCSTR shader_target;
        ID3DBlob *shader_blob;
        void *shader_byte_code;
        size_t shader_byte_code_len;
};

void compile_shader(struct gpu_shader_info *shader_info);
void release_shader(struct gpu_shader_info *shader_info);


struct gpu_vert_input_info {
        UINT attribute_count;
        D3D12_INPUT_ELEMENT_DESC *input_element_descs;
};

void setup_vertex_input(LPCSTR *attribute_names, DXGI_FORMAT *attribute_formats,
        struct gpu_vert_input_info *input_info);
void free_vertex_input(struct gpu_vert_input_info *input_info);


struct gpu_root_param_info {
        D3D12_DESCRIPTOR_RANGE_TYPE range_type;
        UINT num_descriptors;
        D3D12_SHADER_VISIBILITY shader_visbility;
};

struct gpu_root_sig_info {
        WCHAR name[1024];
        ID3DBlob *root_sig_blob;
        ID3D12RootSignature *root_sig;
};

void create_root_sig(struct gpu_device_info *device_info,
        struct gpu_root_param_info *root_params, UINT num_root_params,
        struct gpu_root_sig_info *root_sig_info);
void release_root_sig(struct gpu_root_sig_info *root_sig_info);


enum PSO_TYPE {
        PSO_TYPE_GRAPHICS,
        PSO_TYPE_COMPUTE
};

struct gpu_graphics_pso_info {
        void *vert_shader_byte_code;
        size_t vert_shader_byte_code_len;
        void *pix_shader_byte_code;
        size_t pix_shader_byte_code_len;
        void *dom_shader_byte_code;
        size_t dom_shader_byte_code_len;
        void *hull_shader_byte_code;
        size_t hull_shader_byte_code_len;
        void *geom_shader_byte_code;
        size_t geom_shader_byte_code_len;
        DXGI_FORMAT render_target_format;
        DXGI_FORMAT depth_target_format;
};

struct gpu_compute_pso_info {
        void *comp_shader_byte_code;
        size_t comp_shader_byte_code_len;
        ID3D12PipelineState *compute_pso;
};

struct gpu_pso_info {
        WCHAR name[1024];
        enum PSO_TYPE type;
        union {
                struct gpu_graphics_pso_info graphics_pso_info;
                struct gpu_compute_pso_info compute_pso_info;
        };
        ID3D12PipelineState *pso;
};

void create_pso(struct gpu_device_info *device_info,
        struct gpu_vert_input_info *vert_input_info,
        struct gpu_root_sig_info *root_sig_info,
        struct gpu_pso_info *pso_info);
static void create_graphics_pso(struct gpu_device_info *device_info,
        struct gpu_vert_input_info *vert_input_info,
        struct gpu_root_sig_info *root_sig_info,
        struct gpu_pso_info *pso_info);
static void create_compute_pso(struct gpu_device_info *device_info,
        struct gpu_root_sig_info *root_sig_info,
        struct gpu_pso_info *pso_info);
void release_pso(struct gpu_pso_info *pso_info);


struct gpu_viewport_info {
        float width;
        float height;
        D3D12_VIEWPORT viewport;
};

void create_viewport(struct gpu_viewport_info *viewport_info);


struct gpu_scissor_rect_info {
        D3D12_RECT scissor_rect;
};

void create_scissor_rect(struct gpu_scissor_rect_info *scissor_rect_info);


#endif
