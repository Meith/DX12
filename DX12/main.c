#include "window_interface.h"
#include "gpu_interface.h"
#include "swapchain_inerface.h"
#include "mesh_interface.h"
#include "camera_interface.h"
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

        // Create command queue
        struct gpu_cmd_queue_info direct_queue_info;
        direct_queue_info.type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        create_cmd_queue(&device_info, &direct_queue_info);

        // Create compute queue
        struct gpu_cmd_queue_info compute_queue_info;
        compute_queue_info.type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        create_cmd_queue(&device_info, &compute_queue_info);

        // Create copy queue
        struct gpu_cmd_queue_info copy_queue_info;
        copy_queue_info.type = D3D12_COMMAND_LIST_TYPE_COPY;
        create_cmd_queue(&device_info, &copy_queue_info);

        // Create swapchain
        struct swapchain_info swp_chain_info;
        swp_chain_info.format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swp_chain_info.buffer_count = 2;
        create_swapchain(&wnd_info, &direct_queue_info, &swp_chain_info);
        
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

        // Create fence for synchronizing frames of draw commands
        struct gpu_fence_info render_fence_info;
        render_fence_info.num_fence_value = swp_chain_info.buffer_count;
        create_fence(&device_info, &render_fence_info);
 
        // Create fence for synchronizing copy and direct queue
        struct gpu_fence_info copy_fence_info;
        copy_fence_info.num_fence_value = 1;
        create_fence(&device_info, &copy_fence_info);

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

        signal_gpu(&copy_queue_info, &copy_fence_info, 0);

        wait_for_gpu(&copy_fence_info, 0);

        // Transition gpu shader resource from copy to vertex buffer
        transition_resource(&render_cmd_list_info, &vert_gpu_resource_info,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

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
        vert_shader_info.flags = D3DCOMPILE_DEBUG | 
                                 D3DCOMPILE_SKIP_OPTIMIZATION | 
                                 D3DCOMPILE_WARNINGS_ARE_ERRORS;
        vert_shader_info.shader_target = "vs_5_1";
        compile_shader(&vert_shader_info);

        // Compile pixel shader
        struct gpu_shader_info pix_shader_info;
        pix_shader_info.shader_file = L"shaders\\tri_pix_shader.hlsl";
        pix_shader_info.flags = D3DCOMPILE_DEBUG | 
                                D3DCOMPILE_SKIP_OPTIMIZATION | 
                                D3DCOMPILE_WARNINGS_ARE_ERRORS;
        pix_shader_info.shader_target = "ps_5_1";
        compile_shader(&pix_shader_info);

        // Setup vertex input layout
        #define ATTRIBUTE_COUNT 2
        LPCSTR attribute_names[ATTRIBUTE_COUNT] = { "POSITION", "COLOR" };
        DXGI_FORMAT attribute_formats[ATTRIBUTE_COUNT] = { 
                DXGI_FORMAT_R32G32B32A32_FLOAT,
                DXGI_FORMAT_R32G32B32A32_FLOAT 
        };
        struct gpu_vert_input_info vert_input_info;
        vert_input_info.attribute_count = ATTRIBUTE_COUNT;
        setup_vertex_input(attribute_names, attribute_formats, 
                &vert_input_info);

        // Create root signature
        #define ROOT_PARAM_COUNT 1
        struct gpu_root_param_info root_param_infos[ROOT_PARAM_COUNT];

        root_param_infos[0].range_type = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        root_param_infos[0].num_descriptors = 1;
        root_param_infos[0].shader_visbility = D3D12_SHADER_VISIBILITY_VERTEX;
       
       /* root_param_infos[1].range_type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        root_param_infos[1].num_descriptors = 1;
        root_param_infos[1].shader_visbility = D3D12_SHADER_VISIBILITY_PIXEL;

        root_param_infos[2].range_type =
                D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        root_param_infos[2].num_descriptors = 1;
        root_param_infos[2].shader_visbility =
                D3D12_SHADER_VISIBILITY_PIXEL;*/

        struct gpu_root_sig_info root_sig_info;
        create_root_sig(&device_info, root_param_infos, ROOT_PARAM_COUNT, 
                &root_sig_info);

        // Create pipeline state object
        struct gpu_pso_info render_pso_info;
        render_pso_info.vert_shader_byte_code = 
                vert_shader_info.shader_byte_code;
        render_pso_info.vert_shader_byte_code_len = 
                vert_shader_info.shader_byte_code_len;
        render_pso_info.pix_shader_byte_code = 
                pix_shader_info.shader_byte_code;
        render_pso_info.pix_shader_byte_code_len =
                pix_shader_info.shader_byte_code_len;
        render_pso_info.dom_shader_byte_code = NULL;
        render_pso_info.dom_shader_byte_code_len = 0;
        render_pso_info.hull_shader_byte_code = NULL;
        render_pso_info.hull_shader_byte_code_len = 0;
        render_pso_info.geom_shader_byte_code = NULL;
        render_pso_info.geom_shader_byte_code_len = 0;
        render_pso_info.render_target_format = swp_chain_info.format;
        render_pso_info.depth_target_format = dsv_resource_info.format;
        create_graphics_pso(&device_info, &vert_input_info, &root_sig_info, 
                &render_pso_info);

        // Now we need to create a view port for the screen we are going to be drawing to
        struct gpu_viewport_info viewport_info;
        viewport_info.width = (float) wnd_info.width;
        viewport_info.height = (float) wnd_info.height;
        create_viewport(&viewport_info);

        // Create the scissor rectangle for the pixel operations
        struct gpu_scissor_rect_info scissor_rect_info;
        create_scissor_rect(&scissor_rect_info);

        // Calc project view matrix
        struct camera_info cam_info;
        calc_proj_view_mat(&cam_info);

        // Create constant buffer descriptor
        struct gpu_descriptor_info cbv_descriptor_info;
        cbv_descriptor_info.type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbv_descriptor_info.num_descriptors = 
                root_param_infos[0].num_descriptors;
        cbv_descriptor_info.flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        create_descriptor(&device_info, &cbv_descriptor_info);

        // Create constant buffer resource
        struct gpu_resource_info cbv_resource_info;
        cbv_resource_info.type = D3D12_HEAP_TYPE_UPLOAD;
        cbv_resource_info.dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        cbv_resource_info.width = align_offset(sizeof (cam_info.pv_mat), 256);                ~(256 - 1);
        cbv_resource_info.height = 1;
        cbv_resource_info.mip_levels = 1;
        cbv_resource_info.format = DXGI_FORMAT_UNKNOWN;
        cbv_resource_info.layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        cbv_resource_info.flags = D3D12_RESOURCE_FLAG_NONE;
        cbv_resource_info.current_state = D3D12_RESOURCE_STATE_GENERIC_READ;
        create_resource(&device_info, &cbv_resource_info);

        // Upload constant buffer resource
        upload_resources(&cbv_resource_info, cam_info.pv_mat);

        // Create constant buffer view
        create_constant_buffer_view(&device_info, &cbv_descriptor_info, 
                &cbv_resource_info);

        // Get current back buffer index
        UINT back_buffer_index = get_backbuffer_index(&swp_chain_info);

        //Render loop
        UINT window_msg = WM_NULL;
        do {
                window_msg = window_message_loop();

                transition_resource(&render_cmd_list_info,
                        &rtv_resource_info[back_buffer_index],
                        D3D12_RESOURCE_STATE_RENDER_TARGET);

                // Point render target view cpu handle to correct descriptor in descriptor heap
                update_cpu_handle(&rtv_descriptor_info, back_buffer_index);

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
                        &render_pso_info);

                // Set viewport
                rec_set_viewport_cmd(&render_cmd_list_info, &viewport_info);

                // Set scissor rect
                rec_set_scissor_rect_cmd(&render_cmd_list_info, 
                        &scissor_rect_info);

                // Set primitve
                rec_set_primitive_cmd(&render_cmd_list_info);

                // Set root signature
                rec_set_graphics_root_sig_cmd(&render_cmd_list_info,
                        &root_sig_info);

                // Set descriptor heap for constant buffer
                rec_set_descriptor_heap_cmd(&render_cmd_list_info,
                        &cbv_descriptor_info);

                // Set constant buffer table
                rec_set_graphics_root_descriptor_table_cmd(
                        &render_cmd_list_info, 0, 
                        &cbv_descriptor_info);

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
                        &rtv_resource_info[back_buffer_index], 
                        D3D12_RESOURCE_STATE_PRESENT);

                // Close command list for execution
                close_cmd_list(&render_cmd_list_info);

                // Exexute command list
                execute_cmd_list(&direct_queue_info, &render_cmd_list_info);

                // Present swapchain
                present_swapchain(&swp_chain_info);

                // Signal GPU
                signal_gpu(&direct_queue_info, &render_fence_info, 
                        back_buffer_index);

                // Get the next frame's buffer index
                back_buffer_index = get_backbuffer_index(&swp_chain_info);

                // Wait for gpu to finish previous frame
                wait_for_gpu(&render_fence_info, back_buffer_index);

                // Reset command allocator
                reset_cmd_allocator(&render_cmd_allocator_info, 
                        back_buffer_index);

                // Reset command list
                reset_cmd_list(&render_cmd_allocator_info, 
                        &render_cmd_list_info, back_buffer_index);

        } while (window_msg != WM_QUIT);

        // Wait for GPU to finish up be starting the cleaning
        signal_gpu(&direct_queue_info, &render_fence_info, back_buffer_index);
        wait_for_gpu(&render_fence_info, back_buffer_index);

        // Release grahics pipeline state object
        release_pso(&render_pso_info);

        // Release root signature
        release_root_sig(&root_sig_info);

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

        // Release copy fence
        release_fence(&copy_fence_info);

        // Release render fence
        release_fence(&render_fence_info);

        // Release copy command list
        release_cmd_list(&copy_cmd_list_info);

        // Release render command list
        release_cmd_list(&render_cmd_list_info);

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

        release_cmd_queue(&direct_queue_info);
 
        release_gpu_device(&device_info);

        destroy_window(&wnd_info, hInstance);

        return 0;
}