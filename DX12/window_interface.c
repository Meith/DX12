#include "window_interface.h"
#include "gpu_interface.h"
#include "swapchain_inerface.h"

#include <assert.h>


void create_window(struct window_info *wnd_info, HINSTANCE hInstance,
                  int nCmdShow)
{
        // Fill in window class struct
        WNDCLASSEX wc;
        wc.cbSize = sizeof (WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.cbClsExtra = 1000;
        wc.cbWndExtra = 1000;
        wc.hInstance = hInstance;
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(hInstance, IDC_ARROW);
        wc.hbrBackground = (HBRUSH) COLOR_WINDOW;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = wnd_info->window_class_name;
        wc.hIconSm = NULL;

        // Register window class struct
        RegisterClassEx(&wc);

        // Create the window
        wnd_info->hwnd = CreateWindowEx(0, wc.lpszClassName, 
                wnd_info->window_name, WS_OVERLAPPEDWINDOW, wnd_info->x, 
                wnd_info->y, wnd_info->width, wnd_info->height, NULL, NULL, 
                hInstance, NULL);

	    assert(wnd_info->hwnd);

        // Show window
        ShowWindow(wnd_info->hwnd, nCmdShow);
}

UINT window_message_loop()
{
        MSG queued_msg;
        // Run message loop
        if (PeekMessage(&queued_msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&queued_msg);
                DispatchMessage(&queued_msg);
        }

        return queued_msg.message;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT nonqueued_msg, WPARAM wparam, 
                           LPARAM lparam)
{       
        LONG_PTR *wndproc_data = (LONG_PTR *) 
                GetWindowLongPtr(hwnd, GWLP_USERDATA);

        if (wndproc_data == NULL) 
                return DefWindowProc(hwnd, nonqueued_msg, wparam, lparam);

        struct window_info *wnd_info = (struct window_info *) wndproc_data[0];

        struct gpu_device_info *device_info = (struct gpu_device_info *) 
                wndproc_data[1];

        struct gpu_cmd_queue_info *render_queue_info = 
                (struct gpu_cmd_queue_info *) wndproc_data[2];
        
        struct swapchain_info *swp_chain_info = (struct swapchain_info *) 
                wndproc_data[3];
        
        struct gpu_descriptor_info *rtv_descriptor_info = 
                (struct gpu_descriptor_info *) wndproc_data[4];
       
        struct gpu_resource_info *rtv_resource_info = 
                (struct gpu_resource_info *) wndproc_data[5];
        
        struct gpu_fence_info *fence_info = (struct gpu_fence_info *) 
                wndproc_data[6];
        
        struct gpu_descriptor_info *dsv_descriptor_info = 
                (struct gpu_descriptor_info *) wndproc_data[7];
       
        struct gpu_resource_info *dsv_resource_info = 
                (struct gpu_resource_info *) wndproc_data[8];
        
        switch (nonqueued_msg)
        {
                case WM_DESTROY :
                {
                        PostQuitMessage(0);
                        break;
                }

                case WM_SIZE :
                {
                        resize_window(wnd_info, device_info, render_queue_info,
                                swp_chain_info, rtv_descriptor_info,
                                rtv_resource_info, fence_info, 
                                dsv_descriptor_info, dsv_resource_info);
                        break;
                }

                default :
                {
                        return DefWindowProc(hwnd, nonqueued_msg, wparam, 
                                lparam);
                }
        }   

        return 0;
}

static void resize_window(struct window_info *wnd_info, 
                         struct gpu_device_info *device_info,
                         struct gpu_cmd_queue_info *render_queue_info,
                         struct swapchain_info *swp_chain_info,
                         struct gpu_descriptor_info *rtv_descriptor_info,
                         struct gpu_resource_info *rtv_resource_info,
                         struct gpu_fence_info *fence_info,
                         struct gpu_descriptor_info *dsv_descriptor_info,
                         struct gpu_resource_info *dsv_resource_info)
{
        RECT client_rect;
        GetClientRect(wnd_info->hwnd, &client_rect);
        wnd_info->width = client_rect.right - client_rect.left;
        wnd_info->height = client_rect.bottom - client_rect.top;

        // Wait for GPU to finish up be starting the cleaning
        signal_gpu(render_queue_info, fence_info, 
                swp_chain_info->current_buffer_index);
        wait_for_gpu(fence_info, 
                swp_chain_info->current_buffer_index);

        for (UINT i = 0; i < rtv_descriptor_info->num_descriptors; ++i) {
                release_resource(&rtv_resource_info[i]);
        }

        release_resource(dsv_resource_info);
        
        resize_swapchain(wnd_info, swp_chain_info);

        for (UINT i = 0; i < rtv_descriptor_info->num_descriptors; ++i) {
                rtv_resource_info[i].resource = get_swapchain_buffer(
                        swp_chain_info, i);
                rtv_resource_info[i].format = swp_chain_info->format;
                rtv_resource_info[i].current_state =
                        D3D12_RESOURCE_STATE_PRESENT;
        }
        create_rendertarget_view(device_info, rtv_descriptor_info,
                rtv_resource_info);

        dsv_resource_info->width = wnd_info->width;
        dsv_resource_info->height = wnd_info->height;
        create_resource(device_info, dsv_resource_info);
        create_depthstencil_view(device_info, dsv_descriptor_info,
                dsv_resource_info);
}

void destroy_window(struct window_info *wnd_info, HINSTANCE hInstance)
{
        DestroyWindow(wnd_info->hwnd);

        UnregisterClass(wnd_info->window_class_name, hInstance);
}
