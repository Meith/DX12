#include "window_framework.h"

#include <assert.h>

HWND create_window(struct window_create_info *wci)
{
        // Fill in window class struct
        WNDCLASSEX wc;
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = wci->hinstance;
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(wci->hinstance, IDC_ARROW);
        wc.hbrBackground = (HBRUSH) COLOR_WINDOW;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = wci->window_class_name;
        wc.hIconSm = NULL;

        // Register window class struct
        RegisterClassEx(&wc);

        // Create the window
        HWND hwnd = CreateWindowEx(0, wc.lpszClassName, wci->window_name, WS_OVERLAPPEDWINDOW, wci->x, wci->y, wci->width, wci->height, NULL, NULL, wci->hinstance, NULL);
        assert(hwnd);

        return hwnd;
}

void show_window(struct window_create_info *wci, HWND hwnd)
{
        // Show window
        ShowWindow(hwnd, wci->ncmd_show);
}

void run_window_message_loop(MSG *msg)
{
        // Run message loop
        if (PeekMessage(msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(msg);
                DispatchMessage(msg);
        }

}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
        switch (msg)
        {
                case WM_DESTROY:
                        PostQuitMessage(0);
                        return 0;

                default:
                        return DefWindowProc(hwnd, msg, wparam, lparam);
        }
}

void destroy_window(struct window_create_info *wci, HWND hwnd)
{
        DestroyWindow(hwnd);

        UnregisterClass(wci->window_class_name, wci->hinstance);
}
