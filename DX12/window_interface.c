#include "window_interface.h"
#include "gpu_interface.h"
#include "swapchain_inerface.h"

#include <assert.h>
#include <stdio.h>

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

void resize_window(struct window_info *wnd_info)
{
        RECT client_rect;
        GetClientRect(wnd_info->hwnd, &client_rect);
        wnd_info->width = client_rect.right - client_rect.left;
        wnd_info->height = client_rect.bottom - client_rect.top;
}

UINT window_message_loop()
{
        MSG queued_msg;
        // Run message loop
        if (PeekMessage(&queued_msg, NULL, 0, 0, PM_REMOVE)) {
                
                char str[256];
                sprintf(str, "PeekMessage: %u \n", queued_msg.message);
                OutputDebugString(str);

                TranslateMessage(&queued_msg);
                DispatchMessage(&queued_msg);
        }

        return queued_msg.message;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT nonqueued_msg, WPARAM wparam, LPARAM lparam)
{       
        char str1[256];
        sprintf(str1, "WindowProc: %u \n", nonqueued_msg);
        OutputDebugString(str1);

        switch (nonqueued_msg)
        {
                case WM_DESTROY :
                {
                        PostQuitMessage(0);
                        break;
                }

                case WM_SIZE :
                {
                        HWND hwndTest = FindWindow("DX12WindowClass", "DX12Window");

                        if (hwndTest != NULL)
                        {
                                assert(hwnd == hwndTest);
                        }
                        
                        struct window_info *wnd_info = 
                                (struct window_info *)
                                GetWindowLongPtr(hwnd, 0);
                        if (wnd_info == NULL) break;

                        struct gpu_device_info *device_info = 
                                (struct gpu_device_info *)
                                GetWindowLongPtr(hwnd, 1);
                        if (device_info == NULL) break;

                        struct gpu_cmd_queue_info *render_queue_info = 
                                (struct gpu_cmd_queue_info *)
                                GetWindowLongPtr(hwnd, 2);
                        if (render_queue_info == NULL) break;

                        struct swapchain_info *swp_chain_info = 
                                (struct swapchain_info *)
                                GetWindowLongPtr(hwnd, 3);
                        if (swp_chain_info == NULL) break;

                        struct gpu_descriptor_info *rtv_descriptor_info = 
                                (struct gpu_descriptor_info *)
                                GetWindowLongPtr(hwnd, 4);
                        if (rtv_descriptor_info == NULL) break;

                        struct gpu_resource_info *rtv_resource_info =
                                (struct gpu_resource_info *)
                                GetWindowLongPtr(hwnd, 5);
                        if (rtv_resource_info == NULL) break;

                        struct gpu_fence_info *fence_info = 
                                (struct gpu_fence_info *)
                                GetWindowLongPtr(hwnd, 6);
                        if (fence_info == NULL) break;

                        UINT back_buffer_index = get_backbuffer_index(
                                swp_chain_info);

                        // Wait for GPU to finish up be starting the cleaning
                        signal_gpu(render_queue_info, fence_info, 
                                back_buffer_index);
                        wait_for_gpu(fence_info, back_buffer_index);

                        resize_window(wnd_info);
                        resize_swapchain(wnd_info, swp_chain_info);
                        create_rendertarget_view(device_info, rtv_descriptor_info,
                                rtv_resource_info);
                        break;
                }

                default :
                {
                    char str2[256];
                    sprintf(str2, "DefWindowProc: %u \n", nonqueued_msg);
                    OutputDebugString(str2);

                    return DefWindowProc(hwnd, nonqueued_msg, wparam, lparam);
                }
        }   

        return 0;
}

void destroy_window(struct window_info *wnd_info, HINSTANCE hInstance)
{
        DestroyWindow(wnd_info->hwnd);

        UnregisterClass(wnd_info->window_class_name, hInstance);
}
