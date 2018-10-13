#include "window_interface.h"
#include "gpu_interface.h"
#include "swapchain_inerface.h"
#include "mesh_interface.h"
#include "camera_interface.h"
#include "error.h"


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

        // Create swapchain
        struct swapchain_info swp_chain_info;
        swp_chain_info.format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swp_chain_info.buffer_count = 2;
        create_swapchain(&wnd_info, &direct_queue_info, &swp_chain_info);
        
        // Create swapchain render target descriptor
        struct gpu_descriptor_info rtv_descriptor_info;
        rtv_descriptor_info.type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtv_descriptor_info.num_descriptors = swp_chain_info.buffer_count;
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

        // Create command allocators
        struct gpu_cmd_allocator_info cmd_allocator_info;
        cmd_allocator_info.cmd_list_type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        cmd_allocator_info.cmd_allocator_count = swp_chain_info.buffer_count;
        create_cmd_allocators(&device_info, &cmd_allocator_info);

        // Create command list
        struct gpu_cmd_list_info cmd_list_info;
        cmd_list_info.cmd_list_type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        create_cmd_list(&device_info, &cmd_allocator_info, &cmd_list_info);

        // Create fence
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
        vert_gpu_resource_info.width = sizeof(struct vertex) *
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
        record_copy_buffer_region_cmd(&cmd_list_info, &vert_gpu_resource_info, 
                &vert_upload_resource_info);

        // Transition gpu shader resource from copy to vertex buffer
        transition_resource(&cmd_list_info, &vert_gpu_resource_info, 
                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

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

        struct gpu_resource_info indices_upload_resource_info;
        indices_upload_resource_info.type = D3D12_HEAP_TYPE_UPLOAD;
        indices_upload_resource_info.dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        indices_upload_resource_info.width = sizeof(unsigned int) *
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

        record_copy_buffer_region_cmd(&cmd_list_info, 
                &indices_gpu_resource_info, &indices_upload_resource_info);

        transition_resource(&cmd_list_info, &indices_gpu_resource_info,
                D3D12_RESOURCE_STATE_INDEX_BUFFER);

        // Create depth buffer descriptor 
        struct gpu_descriptor_info dsv_descriptor_info;
        dsv_descriptor_info.type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsv_descriptor_info.num_descriptors = 1;
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
        create_depthstencil_view(&device_info, &dsv_descriptor_info, &dsv_resource_info);

        // Compile vertex shader
        struct gpu_shader_info vert_shader_info;
        vert_shader_info.shader_file = L"shaders\\tri_vert_shader.hlsl";
        vert_shader_info.flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_WARNINGS_ARE_ERRORS;
        vert_shader_info.shader_target = "vs_5_1";
        compile_shader(&vert_shader_info);

        // Compile pixel shader
        struct gpu_shader_info pix_shader_info;
        pix_shader_info.shader_file = L"shaders\\tri_pix_shader.hlsl";
        pix_shader_info.flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_WARNINGS_ARE_ERRORS;
        pix_shader_info.shader_target = "ps_5_1";
        compile_shader(&pix_shader_info);

        // Setup vertex input layout
        struct gpu_vert_input_info vert_input_info;
        vert_input_info.attribute_count = 2;
        vert_input_info.semantic_names = malloc(vert_input_info.attribute_count
                * sizeof (LPCSTR));
        vert_input_info.semantic_names[0] = "POSITION";
        vert_input_info.semantic_names[1] = "COLOR";
        vert_input_info.formats = malloc(vert_input_info.attribute_count
                * sizeof (DXGI_FORMAT));
        vert_input_info.formats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
        vert_input_info.formats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
        setup_vertex_input(&vert_input_info);

        // Create root signature
        struct gpu_root_sig_info root_sig_info;
        root_sig_info.num_root_params = 1;
        root_sig_info.root_param_types = malloc(root_sig_info.num_root_params * 
                sizeof (D3D12_ROOT_PARAMETER_TYPE));
        root_sig_info.root_param_types[0] = 
                D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        root_sig_info.num_32bit_vals_per_const = malloc(
                root_sig_info.num_root_params * sizeof (UINT));
        root_sig_info.num_32bit_vals_per_const[0] = 
                sizeof (mat4x4) / sizeof (float);
        create_root_sig(&device_info, &root_sig_info);

        // Create pipeline state object
        struct gpu_pso_info pso_info;
        pso_info.vert_shader_byte_code = vert_shader_info.shader_byte_code;
        pso_info.vert_shader_byte_code_len = 
                vert_shader_info.shader_byte_code_len;
        pso_info.pix_shader_byte_code = pix_shader_info.shader_byte_code;
        pso_info.pix_shader_byte_code_len =
                pix_shader_info.shader_byte_code_len;
        pso_info.dom_shader_byte_code = NULL;
        pso_info.dom_shader_byte_code_len = 0;
        pso_info.hull_shader_byte_code = NULL;
        pso_info.hull_shader_byte_code_len = 0;
        pso_info.geom_shader_byte_code = NULL;
        pso_info.geom_shader_byte_code_len = 0;
        pso_info.render_target_format = swp_chain_info.format;
        pso_info.depth_target_format = dsv_resource_info.format;
        create_pso(&device_info, &vert_input_info, &root_sig_info, &pso_info);

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

        // Get current back buffer index
        UINT back_buffer_index = get_backbuffer_index(&swp_chain_info);

        //Render loop
        UINT window_msg = WM_NULL;
        do {
                window_msg = window_message_loop();

                transition_resource(&cmd_list_info,
                        &rtv_resource_info[back_buffer_index],
                        D3D12_RESOURCE_STATE_RENDER_TARGET);

                // Point render target view cpu handle to correct descriptor in descriptor heap
                update_cpu_handle(&rtv_descriptor_info, back_buffer_index);

                // Set the render target and depth target
                record_set_render_target_cmd(&cmd_list_info, &rtv_descriptor_info,
                        &dsv_descriptor_info);

                // Clear render target
                float clear_color[] = { 0.4f, 0.6f, 0.9f, 1.0f };
                record_clear_rtv_cmd(&cmd_list_info, &rtv_descriptor_info, 
                        clear_color);

                // Clear depth target
                record_clear_dsv_cmd(&cmd_list_info, &dsv_descriptor_info);

                // Set pipeline state
                record_set_pipeline_state_cmd(&cmd_list_info, &pso_info);

                // Set viewport
                record_set_viewport_cmd(&cmd_list_info, &viewport_info);

                // Set scissor rect
                record_set_scissor_rect_cmd(&cmd_list_info, &scissor_rect_info);

                // Set primitve
                record_set_primitive_cmd(&cmd_list_info);

                // Set root signature
                record_set_graphics_root_sig_cmd(&cmd_list_info, 
                        &root_sig_info);

                // Set root constant
                record_set_root_constansts_cmd(&cmd_list_info, 0, &root_sig_info, 
                        cam_info.pv_mat);

                // Set vertex buffer
                record_set_vertex_buffer_cmd(&cmd_list_info, 
                        &vert_gpu_resource_info, sizeof (struct vertex));

                // Set index buffer
                record_set_index_buffer_cmd(&cmd_list_info, 
                        &indices_gpu_resource_info);

                // Draw indexed instanced
                record_draw_indexed_instance_cmd(&cmd_list_info, 
                        triangle_mesh.index_count, 1);

                // Transition render target buffer to present state
                transition_resource(&cmd_list_info, 
                        &rtv_resource_info[back_buffer_index], 
                        D3D12_RESOURCE_STATE_PRESENT);

                // Close command list for execution
                close_cmd_list(&cmd_list_info);

                // Exexute command list
                execute_cmd_list(&direct_queue_info, &cmd_list_info);

                // Present swapchain
                present_swapchain(&swp_chain_info);

                // Signal GPU
                signal_gpu(&direct_queue_info, &fence_info, back_buffer_index);

                // Get the next frame's buffer index
                back_buffer_index = get_backbuffer_index(&swp_chain_info);

                // Wait for gpu to finish previous frame
                wait_for_gpu(&fence_info, back_buffer_index);

                // Reset command allocator
                reset_cmd_allocator(&cmd_allocator_info, back_buffer_index);

                // Reset command list
                reset_cmd_list(&cmd_allocator_info, &cmd_list_info, back_buffer_index);

        } while (window_msg != WM_QUIT);

        // Wait for GPU to finish up be starting the cleaning
        signal_gpu(&direct_queue_info, &fence_info, back_buffer_index);
        wait_for_gpu(&fence_info, back_buffer_index);

        // Release grahics pipeline state object
        release_pso(&pso_info);

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

        // Release fence
        release_fence(&fence_info);

        // Release command list
        release_cmd_list(&cmd_list_info);

        // Release command allocator
        release_cmd_allocators(&cmd_allocator_info);

        // Release the render target view buffers
        for (UINT i = 0; i < rtv_descriptor_info.num_descriptors; ++i) {
                release_resource(&rtv_resource_info[i]);
        }

        free(rtv_resource_info);

        // Release render target view descriptor heap
        release_descriptor(&rtv_descriptor_info);

        release_swapchain(&swp_chain_info);

        release_cmd_queue(&direct_queue_info);
 
        release_gpu_device(&device_info);

        destroy_window(&wnd_info, hInstance);

        return 0;
}