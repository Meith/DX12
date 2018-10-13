#ifndef WINDOW_INTERFACE_H
#define WINDOW_INTERFACE_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct window_info {
        const char *window_class_name;
        const char *window_name;
        UINT x;
        UINT y;
        UINT width;
        UINT height;
        HWND hwnd;
};

void create_window(struct window_info *wnd_info, HINSTANCE hInstance,
                  int nCmdShow);
UINT window_message_loop();
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
void destroy_window(struct window_info *wnd_info, HINSTANCE hInstance);

#endif
