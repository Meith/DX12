#include "window_interface.h"

#include <assert.h>

void create_window(struct window_info *wnd_info, HINSTANCE hInstance,
                  int nCmdShow)
{
        // Fill in window class struct
        WNDCLASSEX wc;
        wc.cbSize = sizeof (WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
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
        MSG msg;
        // Run message loop
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                switch (msg.message)
                {
                        case WM_SIZE :
                                break;
                        default :
                        {
                                TranslateMessage(&msg);
                                DispatchMessage(&msg);
                        }
                }
        }

        return msg.message;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{        
        switch (msg)
        {
                case WM_DESTROY :
                {
                        PostQuitMessage(0);
                        break;
                }

                case WM_SIZE:
                        break;

                default :
                        return DefWindowProc(hwnd, msg, wparam, lparam);
        }   

        return 0;
}

void destroy_window(struct window_info *wnd_info, HINSTANCE hInstance)
{
        DestroyWindow(wnd_info->hwnd);

        UnregisterClass(wnd_info->window_class_name, hInstance);
}
