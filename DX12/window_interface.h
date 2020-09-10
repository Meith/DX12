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
LRESULT CALLBACK WindowProc(HWND hwnd, UINT nonqueued_msg, WPARAM wparam,
        LPARAM lparam);
static void resize_window(struct window_info *wnd_info,
        struct gpu_device_info *device_info,
        struct gpu_cmd_queue_info *render_queue_info,
        struct swapchain_info *swp_chain_info,
        struct gpu_descriptor_info *rtv_descriptor_info,
        struct gpu_resource_info *rtv_resource_info,
        struct gpu_descriptor_info *tmp_rtv_descriptor_info,
        struct gpu_resource_info *tmp_rtv_resource_info,
        struct gpu_fence_info *fence_info,
        struct gpu_descriptor_info *dsv_descriptor_info,
        struct gpu_resource_info *dsv_resource_info,
        struct gpu_root_param_info *compute_root_param_infos,
        struct gpu_descriptor_info *compute_cbv_srv_uav_descriptor_info,
        UINT *num_compute_cbv_srv_uav_descriptors);
void destroy_window(struct window_info *wnd_info, HINSTANCE hInstance);

#endif
