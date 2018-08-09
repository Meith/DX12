#ifndef WINDOW_FRAMEWORK
#define WINDOW_FRAMEWORK

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct window_create_info {
        HINSTANCE hinstance;
        const char *window_class_name;
        const char *window_name;
        UINT x;
        UINT y;
        UINT width;
        UINT height;
        int ncmd_show;
};

HWND create_window(struct window_create_info *wci);
void show_window(struct window_create_info *wci, HWND hwnd);
void run_window_message_loop(MSG *msg);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
void destroy_window(struct window_create_info *wci, HWND hwnd);

#endif
