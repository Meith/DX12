#ifndef SWAPCHAIN_INTERFACE_H
#define SWAPCHAIN_INTERFACE_H

#include "gpu_interface.h"

#define COBJMACROS
#include <dxgi1_5.h>


struct swapchain_info {
        DXGI_FORMAT format;
        UINT buffer_count;
        UINT current_buffer_index;
        IDXGIFactory5 *factory5;
        IDXGISwapChain4 *swapchain4;
};

void create_swapchain(struct window_info *wnd_info,
        struct gpu_cmd_queue_info *cmd_queue_info,
        struct swapchain_info *swp_chain_info);
void resize_swapchain(struct window_info *wnd_info,
        struct swapchain_info *swp_chain_info);
void set_fullscreen_swapchain(BOOL is_fullscreen,
        struct swapchain_info *swp_chain_info);
UINT get_backbuffer_index(struct swapchain_info *swp_chain_info);
ID3D12Resource *get_swapchain_buffer(struct swapchain_info *swp_chain_info,
        UINT buffer_index);
void present_swapchain(struct swapchain_info *swp_chain_info);
void release_swapchain(struct swapchain_info *swp_chain_info);

#endif