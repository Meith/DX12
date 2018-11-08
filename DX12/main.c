#include "window_interface.h"
#include "gpu_interface.h"
#include "swapchain_inerface.h"
#include "mesh_interface.h"
#include "camera_interface.h"
#include "material_interface.h"
#include "error.h"
#include "misc.h"


int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                    LPSTR lpCmdLine, int nCmdShow)
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
        render_queue_info.type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        create_cmd_queue(&device_info, &render_queue_info);

        // Create compute queue
        struct gpu_cmd_queue_info compute_queue_info;
        compute_queue_info.type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        create_cmd_queue(&device_info, &compute_queue_info);

        // Create copy queue
        struct gpu_cmd_queue_info copy_queue_info;
        copy_queue_info.type = D3D12_COMMAND_LIST_TYPE_COPY;
        create_cmd_queue(&device_info, &copy_queue_info);

        // Create swapchain
        struct swapchain_info swp_chain_info;
        swp_chain_info.format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swp_chain_info.buffer_count = 2;
        create_swapchain(&wnd_info, &render_queue_info, &swp_chain_info);

        // Create swapchain render target descriptor
        struct gpu_descriptor_info rtv_descriptor_info;
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
        create_rendertarget_view(&device_info, &rtv_descriptor_info,
                rtv_resource_info);

        // Create command allocators for draw commands
        struct gpu_cmd_allocator_info render_cmd_allocator_info;
        render_cmd_allocator_info.cmd_list_type = 
                D3D12_COMMAND_LIST_TYPE_DIRECT;
        render_cmd_allocator_info.cmd_allocator_count = 
                swp_chain_info.buffer_count;
        create_cmd_allocators(&device_info, &render_cmd_allocator_info);

        // Create command list for draw commands
        struct gpu_cmd_list_info render_cmd_list_info;
        render_cmd_list_info.cmd_list_type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        create_cmd_list(&device_info, &render_cmd_allocator_info, 
                &render_cmd_list_info);

        // Create command allocators for copy commands
        struct gpu_cmd_allocator_info copy_cmd_allocator_info;
        copy_cmd_allocator_info.cmd_list_type = D3D12_COMMAND_LIST_TYPE_COPY;
        copy_cmd_allocator_info.cmd_allocator_count = 1;
        create_cmd_allocators(&device_info, &copy_cmd_allocator_info);

        // Create command list for copy commands
        struct gpu_cmd_list_info copy_cmd_list_info;
        copy_cmd_list_info.cmd_list_type = D3D12_COMMAND_LIST_TYPE_COPY;
        create_cmd_list(&device_info, &copy_cmd_allocator_info, 
                &copy_cmd_list_info);

        // Create command allocators for copy commands
        struct gpu_cmd_allocator_info compute_cmd_allocator_info;
        compute_cmd_allocator_info.cmd_list_type = 
                D3D12_COMMAND_LIST_TYPE_COMPUTE;
        compute_cmd_allocator_info.cmd_allocator_count = 1;
        create_cmd_allocators(&device_info, &compute_cmd_allocator_info);

        // Create command list for copy commands
        struct gpu_cmd_list_info compute_cmd_list_info;
        compute_cmd_list_info.cmd_list_type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        create_cmd_list(&device_info, &compute_cmd_allocator_info,
                &compute_cmd_list_info);

        // Create fence for synchronizing
        struct gpu_fence_info fence_info;
        fence_info.num_fence_value = swp_chain_info.buffer_count;
        create_fence(&device_info, &fence_info);

        // Create triangle mesh
        struct mesh_info triangle_mesh;
        create_triangle(&triangle_mesh);

        // Create triangle resource
        // First resource for vertices on the GPU for shader usage
        struct gpu_resource_info vert_gpu_resource_info;
        vert_gpu_resource_info.type = D3D12_HEAP_TYPE_DEFAULT;
        vert_gpu_resource_info.dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        vert_gpu_resource_info.width = sizeof (struct vertex) *
                triangle_mesh.vertex_count;
        vert_gpu_resource_info.height = 1;
        vert_gpu_resource_info.mip_levels = 1;
        vert_gpu_resource_info.format = DXGI_FORMAT_UNKNOWN;
        vert_gpu_resource_info.layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        vert_gpu_resource_info.flags = D3D12_RESOURCE_FLAG_NONE;
        vert_gpu_resource_info.current_state = D3D12_RESOURCE_STATE_COPY_DEST;
        create_resource(&device_info, &vert_gpu_resource_info);

        // Reource for uploading vertices from CPU to GPU
        struct gpu_resource_info vert_upload_resource_info;
        vert_upload_resource_info.type = D3D12_HEAP_TYPE_UPLOAD;
        vert_upload_resource_info.dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        vert_upload_resource_info.width = sizeof (struct vertex) *
                triangle_mesh.vertex_count;
        vert_upload_resource_info.height = 1;
        vert_upload_resource_info.mip_levels = 1;
        vert_upload_resource_info.format = DXGI_FORMAT_UNKNOWN;
        vert_upload_resource_info.layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        vert_upload_resource_info.flags = D3D12_RESOURCE_FLAG_NONE;
        vert_upload_resource_info.current_state =
                D3D12_RESOURCE_STATE_GENERIC_READ;
        create_resource(&device_info, &vert_upload_resource_info);

        // Copy triangle vertex data from cpu to upload resource
        upload_resources(&vert_upload_resource_info, triangle_mesh.verticies);

        // Copy vertex data from upload resource to gpu shader resource
        rec_copy_buffer_region_cmd(&copy_cmd_list_info, 
                &vert_gpu_resource_info, &vert_upload_resource_info);

        // Resource for index buffer on the GPU for shader usage
        struct gpu_resource_info indices_gpu_resource_info;
        indices_gpu_resource_info.type = D3D12_HEAP_TYPE_DEFAULT;
        indices_gpu_resource_info.dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        indices_gpu_resource_info.width = sizeof (unsigned int) *
                triangle_mesh.index_count;
        indices_gpu_resource_info.height = 1;
        indices_gpu_resource_info.mip_levels = 1;
        indices_gpu_resource_info.format = DXGI_FORMAT_UNKNOWN;
        indices_gpu_resource_info.layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        indices_gpu_resource_info.flags = D3D12_RESOURCE_FLAG_NONE;
        indices_gpu_resource_info.current_state =
                D3D12_RESOURCE_STATE_COPY_DEST;
        create_resource(&device_info, &indices_gpu_resource_info);

        // Resource for uploading indices from CPU to GPU
        struct gpu_resource_info indices_upload_resource_info;
        indices_upload_resource_info.type = D3D12_HEAP_TYPE_UPLOAD;
        indices_upload_resource_info.dimension = 
                D3D12_RESOURCE_DIMENSION_BUFFER;
        indices_upload_resource_info.width = sizeof (unsigned int) *
                triangle_mesh.index_count;
        indices_upload_resource_info.height = 1;
        indices_upload_resource_info.mip_levels = 1;
        indices_upload_resource_info.format = DXGI_FORMAT_UNKNOWN;
        indices_upload_resource_info.layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        indices_upload_resource_info.flags = D3D12_RESOURCE_FLAG_NONE;
        indices_upload_resource_info.current_state =
                D3D12_RESOURCE_STATE_GENERIC_READ;
        create_resource(&device_info, &indices_upload_resource_info);

        upload_resources(&indices_upload_resource_info, triangle_mesh.indices);

        rec_copy_buffer_region_cmd(&copy_cmd_list_info,
                &indices_gpu_resource_info, &indices_upload_resource_info);

        // Close command list for execution
        close_cmd_list(&copy_cmd_list_info);

        // Exexute command list
        execute_cmd_list(&copy_queue_info, &copy_cmd_list_info);

        // Signal gpu regarding copy queue work
        signal_gpu(&copy_queue_info, &fence_info, 
                swp_chain_info.current_buffer_index);

        // Transition gpu vertex shader resource from copy to vertex buffer
        transition_resource(&render_cmd_list_info, &vert_gpu_resource_info,
                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        // Transition gpu index shader resource from copy to index buffer
        transition_resource(&render_cmd_list_info, &indices_gpu_resource_info,
                D3D12_RESOURCE_STATE_INDEX_BUFFER);

        // Create depth buffer descriptor 
        struct gpu_descriptor_info dsv_descriptor_info;
        dsv_descriptor_info.type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsv_descriptor_info.num_descriptors = 1;
        dsv_descriptor_info.flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        create_descriptor(&device_info, &dsv_descriptor_info);

        // Create depth buffer resource
        struct gpu_resource_info dsv_resource_info;
        dsv_resource_info.type = D3D12_HEAP_TYPE_DEFAULT;
        dsv_resource_info.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        dsv_resource_info.width = wnd_info.width;
        dsv_resource_info.height = wnd_info.height;
        dsv_resource_info.mip_levels = 0;
        dsv_resource_info.format = DXGI_FORMAT_D32_FLOAT;
        dsv_resource_info.layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        dsv_resource_info.flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        dsv_resource_info.current_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        create_resource(&device_info, &dsv_resource_info);
        
        // Create depth buffer view
        create_depthstencil_view(&device_info, &dsv_descriptor_info, 
                &dsv_resource_info);

        // Compile vertex shader
        struct gpu_shader_info vert_shader_info;
        vert_shader_info.shader_file = L"shaders\\tri_vert_shader.hlsl";
        vert_shader_info.shader_target = "vs_5_1";
        compile_shader(&vert_shader_info);

        // Compile pixel shader
        struct gpu_shader_info pix_shader_info;
        pix_shader_info.shader_file = L"shaders\\tri_pix_shader.hlsl";
        pix_shader_info.shader_target = "ps_5_1";
        compile_shader(&pix_shader_info);

        // Setup vertex input layout
        #define ATTRIBUTE_COUNT 3
        LPCSTR attribute_names[ATTRIBUTE_COUNT] = { "POSITION", "COLOR", 
                "TEXCOORD" };
        DXGI_FORMAT attribute_formats[ATTRIBUTE_COUNT] = { 
                DXGI_FORMAT_R32G32B32A32_FLOAT,
                DXGI_FORMAT_R32G32B32A32_FLOAT,
                DXGI_FORMAT_R32G32_FLOAT
        };

        struct gpu_vert_input_info vert_input_info;
        vert_input_info.attribute_count = ATTRIBUTE_COUNT;
        setup_vertex_input(attribute_names, attribute_formats, 
                &vert_input_info);

        // Create root signature
        struct gpu_root_param_info graphics_root_param_infos[3];

        graphics_root_param_infos[0].range_type = 
                D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        graphics_root_param_infos[0].num_descriptors = 1;
        graphics_root_param_infos[0].shader_visbility = 
                D3D12_SHADER_VISIBILITY_VERTEX;
       
        graphics_root_param_infos[1].range_type = 
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        graphics_root_param_infos[1].num_descriptors = 1;
        graphics_root_param_infos[1].shader_visbility = 
                D3D12_SHADER_VISIBILITY_PIXEL;

        graphics_root_param_infos[2].range_type =
                D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        graphics_root_param_infos[2].num_descriptors = 1;
        graphics_root_param_infos[2].shader_visbility =
                D3D12_SHADER_VISIBILITY_PIXEL;

        struct gpu_root_sig_info graphics_root_sig_info;
        create_root_sig(&device_info, graphics_root_param_infos, 3,
                &graphics_root_sig_info);

        // Create graphics pipeline state object
        struct gpu_pso_info graphics_pso_info;
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
                swp_chain_info.format;
        graphics_pso_info.graphics_pso_info.depth_target_format = 
                dsv_resource_info.format;
        create_pso(&device_info, &vert_input_info, &graphics_root_sig_info,
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
        calc_proj_view_mat(&cam_info);

        // Create constant buffer and texture resource descriptor
        struct gpu_descriptor_info graphics_cbv_srv_uav_descriptor_info;
        graphics_cbv_srv_uav_descriptor_info.type = 
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        graphics_cbv_srv_uav_descriptor_info.num_descriptors =
                graphics_root_param_infos[0].num_descriptors + 
                graphics_root_param_infos[1].num_descriptors;
        graphics_cbv_srv_uav_descriptor_info.flags =
                D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        create_descriptor(&device_info, &graphics_cbv_srv_uav_descriptor_info);

        // Create constant buffer resource
        struct gpu_resource_info graphics_cbv_resource_info;
        graphics_cbv_resource_info.type = D3D12_HEAP_TYPE_UPLOAD;
        graphics_cbv_resource_info.dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        graphics_cbv_resource_info.width = 
                align_offset(sizeof (cam_info.pv_mat), 256);
        graphics_cbv_resource_info.height = 1;
        graphics_cbv_resource_info.mip_levels = 1;
        graphics_cbv_resource_info.format = DXGI_FORMAT_UNKNOWN;
        graphics_cbv_resource_info.layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        graphics_cbv_resource_info.flags = D3D12_RESOURCE_FLAG_NONE;
        graphics_cbv_resource_info.current_state = 
                D3D12_RESOURCE_STATE_GENERIC_READ;
        create_resource(&device_info, &graphics_cbv_resource_info);

        // Upload constant buffer resource
        upload_resources(&graphics_cbv_resource_info, cam_info.pv_mat);

        // Create constant buffer view
        create_constant_buffer_view(&device_info, 
                &graphics_cbv_srv_uav_descriptor_info, 
                &graphics_cbv_resource_info);

        // Update descriptor heap pointer to point to texture resource
        update_cpu_handle(&graphics_cbv_srv_uav_descriptor_info, 1);

        // Create texture resource
        struct gpu_resource_info tex_resource_info;
        tex_resource_info.type = D3D12_HEAP_TYPE_DEFAULT;
        tex_resource_info.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        tex_resource_info.width = 256;
        tex_resource_info.height = 256;
        tex_resource_info.mip_levels = 1;
        tex_resource_info.format = DXGI_FORMAT_R8G8B8A8_UNORM;
        tex_resource_info.layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        tex_resource_info.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        tex_resource_info.current_state = D3D12_RESOURCE_STATE_COPY_DEST;
        create_resource(&device_info, &tex_resource_info);

        // Get checker board texture material
        struct material_info checkerboard_mat_info;
        get_checkerboard_tex(tex_resource_info.width, tex_resource_info.height,
                &checkerboard_mat_info);

        // Create texture upload resource
        struct gpu_resource_info tex_upload_resource_info;
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

        // Create shader resource view
        create_shader_resource_view(&device_info, 
                &graphics_cbv_srv_uav_descriptor_info, &tex_resource_info);

        // Create unordered access view
        create_unorderd_access_view(&device_info, 
                &graphics_cbv_srv_uav_descriptor_info, &tex_resource_info);

        // Create sampler descriptor
        struct gpu_descriptor_info sampler_descriptor_info;
        sampler_descriptor_info.type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        sampler_descriptor_info.num_descriptors =
                graphics_root_param_infos[2].num_descriptors;
        sampler_descriptor_info.flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        create_descriptor(&device_info, &sampler_descriptor_info);

        // Create sampler
        create_sampler(&device_info, &sampler_descriptor_info);

        // Upload texture resource
        upload_resources(&tex_upload_resource_info, checkerboard_mat_info.tex);

        // Make sure vertex and index upload is done before copy command allocator and list is reset
        wait_for_gpu(&fence_info, swp_chain_info.current_buffer_index);

        reset_cmd_allocators(&copy_cmd_allocator_info);

        reset_cmd_list(&copy_cmd_allocator_info, &copy_cmd_list_info, 0);

        rec_copy_texture_region_cmd(&copy_cmd_list_info, &tex_resource_info,
                &tex_upload_resource_info);

        // Transition texture shader resource to read/write buffer
        transition_resource(&copy_cmd_list_info, &tex_resource_info,
                D3D12_RESOURCE_STATE_COMMON);

        // Close command list for execution
        close_cmd_list(&copy_cmd_list_info);

        // Exexute command list
        execute_cmd_list(&copy_queue_info, &copy_cmd_list_info);

        // Signal GPU for texture upload of copy queue work
        signal_gpu(&copy_queue_info, &fence_info, 
                swp_chain_info.current_buffer_index);

        // Make sure compute queue which will use the texture waits for it be uploaded
        wait_for_fence(&compute_queue_info, &fence_info, 
                swp_chain_info.current_buffer_index);

        // Compile compute shader
        struct gpu_shader_info comp_shader_info;
        comp_shader_info.shader_file = L"shaders\\tri_comp_shader.hlsl";
        comp_shader_info.shader_target = "cs_5_1";
        compile_shader(&comp_shader_info);

        // Create compute root signature
        struct gpu_root_param_info compute_root_param_infos[2];

        compute_root_param_infos[0].range_type =
                D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        compute_root_param_infos[0].num_descriptors = 1;
        compute_root_param_infos[0].shader_visbility =
                D3D12_SHADER_VISIBILITY_ALL;

        compute_root_param_infos[1].range_type =
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        compute_root_param_infos[1].num_descriptors = 1;
        compute_root_param_infos[1].shader_visbility =
                D3D12_SHADER_VISIBILITY_ALL;

        struct gpu_root_sig_info compute_root_sig_info;
        create_root_sig(&device_info, compute_root_param_infos, 2,
                &compute_root_sig_info);

        // Create compute pipeline state object
        struct gpu_pso_info compute_pso_info;
        compute_pso_info.type = PSO_TYPE_COMPUTE;
        compute_pso_info.compute_pso_info.comp_shader_byte_code =
                comp_shader_info.shader_byte_code;
        compute_pso_info.compute_pso_info.comp_shader_byte_code_len =
                comp_shader_info.shader_byte_code_len;
        create_pso(&device_info, NULL, &compute_root_sig_info,
                &compute_pso_info);

        // Create constant buffer descriptor for compute
        struct gpu_descriptor_info compute_cbv_srv_uav_descriptor_info;
        compute_cbv_srv_uav_descriptor_info.type =
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        compute_cbv_srv_uav_descriptor_info.num_descriptors =
                compute_root_param_infos[0].num_descriptors + 
                compute_root_param_infos[1].num_descriptors;
        compute_cbv_srv_uav_descriptor_info.flags =
                D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        create_descriptor(&device_info, &compute_cbv_srv_uav_descriptor_info);

        // Create constant buffer resource
        struct gpu_resource_info compute_cbv_resource_info;
        compute_cbv_resource_info.type = D3D12_HEAP_TYPE_UPLOAD;
        compute_cbv_resource_info.dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        compute_cbv_resource_info.width =
                align_offset(sizeof (float), 256);
        compute_cbv_resource_info.height = 1;
        compute_cbv_resource_info.mip_levels = 1;
        compute_cbv_resource_info.format = DXGI_FORMAT_UNKNOWN;
        compute_cbv_resource_info.layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        compute_cbv_resource_info.flags = D3D12_RESOURCE_FLAG_NONE;
        compute_cbv_resource_info.current_state =
                D3D12_RESOURCE_STATE_GENERIC_READ;
        create_resource(&device_info, &compute_cbv_resource_info);

        time_t previous_time_in_sec = time_in_secs();

        // Create constant buffer view
        create_constant_buffer_view(&device_info,
                &compute_cbv_srv_uav_descriptor_info, 
                &compute_cbv_resource_info);
     
        update_cpu_handle(&compute_cbv_srv_uav_descriptor_info, 1);

        // Create unordered access view
        create_unorderd_access_view(&device_info, 
                &compute_cbv_srv_uav_descriptor_info, &tex_resource_info);

        LONG_PTR wndproc_data[] = { (LONG_PTR) &wnd_info, 
                (LONG_PTR) &device_info, (LONG_PTR) &render_queue_info,
                (LONG_PTR) &swp_chain_info, (LONG_PTR) &rtv_descriptor_info,
                (LONG_PTR) rtv_resource_info, (LONG_PTR) &fence_info,
                (LONG_PTR) &dsv_descriptor_info, (LONG_PTR) &dsv_resource_info
        };
        SetWindowLongPtr(wnd_info.hwnd, GWLP_USERDATA, (LONG_PTR) wndproc_data);

        UINT queued_window_msg = WM_NULL;
        do {
                queued_window_msg = window_message_loop();

                time_t current_time_in_sec = time_in_secs();
                time_t sec = current_time_in_sec - previous_time_in_sec;

                float float_sec = (float) sec;

                upload_resources(&compute_cbv_resource_info, &float_sec);

                // Transition texture shader resource to read/write buffer
                transition_resource(&compute_cmd_list_info, &tex_resource_info,
                        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

                // Set pipeline state
                rec_set_pipeline_state_cmd(&compute_cmd_list_info,
                        &compute_pso_info);

                // Set root signature
                rec_set_compute_root_sig_cmd(&compute_cmd_list_info,
                        &compute_root_sig_info);

                // Set descriptor heap for compute constant buffer
                rec_set_descriptor_heap_cmd(&compute_cmd_list_info,
                        &compute_cbv_srv_uav_descriptor_info);

                update_gpu_handle(&compute_cbv_srv_uav_descriptor_info, 0);

                // Set constant buffer table
                rec_set_compute_root_descriptor_table_cmd(
                        &compute_cmd_list_info, 0,
                        &compute_cbv_srv_uav_descriptor_info);

                update_gpu_handle(&compute_cbv_srv_uav_descriptor_info, 1);

                // Set shader resource table
                rec_set_compute_root_descriptor_table_cmd(
                        &compute_cmd_list_info, 1,
                        &compute_cbv_srv_uav_descriptor_info);

                // Call compute dispatch
                rec_dispatch_cmd(&compute_cmd_list_info, 
                        (UINT) tex_resource_info.width / 8, 
                        (UINT) tex_resource_info.height / 8, 1);

                // Close command list for execution
                close_cmd_list(&compute_cmd_list_info);

                // Exexute command list
                execute_cmd_list(&compute_queue_info, &compute_cmd_list_info);

                signal_gpu(&compute_queue_info, &fence_info, 
                        swp_chain_info.current_buffer_index);

                wait_for_fence(&render_queue_info, &fence_info, 
                        swp_chain_info.current_buffer_index);

                // Transition texture shader resource to read/write buffer
                transition_resource(&render_cmd_list_info, &tex_resource_info,
                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

                transition_resource(&render_cmd_list_info,
                        &rtv_resource_info[swp_chain_info.current_buffer_index],
                        D3D12_RESOURCE_STATE_RENDER_TARGET);

                // Point render target view cpu handle to correct descriptor in descriptor heap
                update_cpu_handle(&rtv_descriptor_info, 
                        swp_chain_info.current_buffer_index);

                // Set the render target and depth target
                rec_set_render_target_cmd(&render_cmd_list_info, 
                        &rtv_descriptor_info, &dsv_descriptor_info);

                // Clear render target
                float clear_color[] = { 0.4f, 0.6f, 0.9f, 1.0f };
                rec_clear_rtv_cmd(&render_cmd_list_info, 
                        &rtv_descriptor_info, clear_color);

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
                rec_set_primitive_cmd(&render_cmd_list_info);

                // Set root signature
                rec_set_graphics_root_sig_cmd(&render_cmd_list_info,
                        &graphics_root_sig_info);

                // Set descriptor heap for constant buffer and texture resource
                rec_set_descriptor_heap_cmd(&render_cmd_list_info,
                        &graphics_cbv_srv_uav_descriptor_info);

                update_gpu_handle(&graphics_cbv_srv_uav_descriptor_info, 0);

                // Set constant buffer table
                rec_set_graphics_root_descriptor_table_cmd(
                        &render_cmd_list_info, 0, 
                        &graphics_cbv_srv_uav_descriptor_info);

                update_gpu_handle(&graphics_cbv_srv_uav_descriptor_info, 1);

                // Set shader resource table
                rec_set_graphics_root_descriptor_table_cmd(
                        &render_cmd_list_info, 1,
                        &graphics_cbv_srv_uav_descriptor_info);

                // Set descriptor heap for sampler
                rec_set_descriptor_heap_cmd(&render_cmd_list_info,
                        &sampler_descriptor_info);

                // Set sampler table
                rec_set_graphics_root_descriptor_table_cmd(
                        &render_cmd_list_info, 2,
                        &sampler_descriptor_info);

                // Set vertex buffer
                rec_set_vertex_buffer_cmd(&render_cmd_list_info,
                        &vert_gpu_resource_info, sizeof (struct vertex));

                // Set index buffer
                rec_set_index_buffer_cmd(&render_cmd_list_info,
                        &indices_gpu_resource_info);

                // Draw indexed instanced
                rec_draw_indexed_instance_cmd(&render_cmd_list_info,
                        triangle_mesh.index_count, 1);

                // Transition render target buffer to present state
                transition_resource(&render_cmd_list_info,
                        &rtv_resource_info[swp_chain_info.current_buffer_index],
                        D3D12_RESOURCE_STATE_PRESENT);

                transition_resource(&render_cmd_list_info, &tex_resource_info,
                        D3D12_RESOURCE_STATE_COMMON);

                // Close command list for execution
                close_cmd_list(&render_cmd_list_info);

                // Exexute command list
                execute_cmd_list(&render_queue_info, &render_cmd_list_info);

                // Present swapchain
                present_swapchain(&swp_chain_info);

                // Signal GPU
                signal_gpu(&render_queue_info, &fence_info,
                        swp_chain_info.current_buffer_index);

                wait_for_fence(&compute_queue_info, &fence_info, 
                        swp_chain_info.current_buffer_index);

                // Wait for gpu to finish previous frame
                wait_for_gpu(&fence_info, swp_chain_info.current_buffer_index);

                // Reset command allocator
                reset_cmd_allocator(&render_cmd_allocator_info, 
                        swp_chain_info.current_buffer_index);

                // Reset command list
                reset_cmd_list(&render_cmd_allocator_info, 
                        &render_cmd_list_info, 
                        swp_chain_info.current_buffer_index);

                // Reset command allocator
                reset_cmd_allocators(&compute_cmd_allocator_info);

                // Reset command list
                reset_cmd_list(&compute_cmd_allocator_info,
                        &compute_cmd_list_info, 0);

        } while (queued_window_msg != WM_QUIT);

        // Wait for GPU to finish up be starting the cleaning
        signal_gpu(&render_queue_info, &fence_info, 
                swp_chain_info.current_buffer_index);

        wait_for_gpu(&fence_info, swp_chain_info.current_buffer_index);

        release_resource(&compute_cbv_resource_info);

        release_descriptor(&compute_cbv_srv_uav_descriptor_info);

        release_descriptor(&sampler_descriptor_info);

        release_resource(&tex_resource_info);

        release_resource(&tex_upload_resource_info);

        release_material(&checkerboard_mat_info);

        release_resource(&graphics_cbv_resource_info);

        release_descriptor(&graphics_cbv_srv_uav_descriptor_info);

        // Release compute pipline state object
        release_pso(&compute_pso_info);

        // Release grahics pipeline state object
        release_pso(&graphics_pso_info);

        release_root_sig(&compute_root_sig_info);

        // Release root signature
        release_root_sig(&graphics_root_sig_info);

        // Release compute shader
        release_shader(&comp_shader_info);

        // Release pixel shader
        release_shader(&pix_shader_info);

        // Release vertex shader
        release_shader(&vert_shader_info);

        // Release depth stencil buffer resource
        release_resource(&dsv_resource_info);

        // Release depth stencl buffer heap
        release_descriptor(&dsv_descriptor_info);

        // Release index buffer upload resource
        release_resource(&indices_upload_resource_info);

        // Release index buffer resource
        release_resource(&indices_gpu_resource_info);

        // Release vertex buffer upload resource
        release_resource(&vert_upload_resource_info);

        // Release vertex buffer resource
        release_resource(&vert_gpu_resource_info);

        // Release triangle data 
        release_triangle(&triangle_mesh);

        // Release render fence
        release_fence(&fence_info);

        // Release compute command list
        release_cmd_list(&compute_cmd_list_info);

        // Release copy command list
        release_cmd_list(&copy_cmd_list_info);

        // Release render command list
        release_cmd_list(&render_cmd_list_info);

        // Release compute command allocator
        release_cmd_allocators(&compute_cmd_allocator_info);

        // Release copy command allocator
        release_cmd_allocators(&copy_cmd_allocator_info);

        // Release render command allocator
        release_cmd_allocators(&render_cmd_allocator_info);

        // Release the render target view buffers
        for (UINT i = 0; i < rtv_descriptor_info.num_descriptors; ++i) {
                release_resource(&rtv_resource_info[i]);
        }

        free(rtv_resource_info);

        // Release render target view descriptor heap
        release_descriptor(&rtv_descriptor_info);

        release_swapchain(&swp_chain_info);

        release_cmd_queue(&copy_queue_info);

        release_cmd_queue(&compute_queue_info);

        release_cmd_queue(&render_queue_info);
 
        release_gpu_device(&device_info);

        destroy_window(&wnd_info, hInstance);

        return 0;
}