#include "swapchain_inerface.h"
#include "error.h"

#pragma comment (lib, "dxgi.lib")

void create_swapchain(struct window_info *wnd_info, 
                     struct gpu_cmd_queue_info *cmd_queue_info,
                     struct swapchain_info *swp_chain_info)
{
        // Describe swapchain
        DXGI_SWAP_CHAIN_DESC1 desc1;
        desc1.Width = wnd_info->width;
        desc1.Height = wnd_info->height;
        desc1.Format = swp_chain_info->format;
        desc1.Stereo = FALSE;
        desc1.SampleDesc.Count = 1;
        desc1.SampleDesc.Quality = 0;
        desc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc1.BufferCount = swp_chain_info->buffer_count;
        desc1.Scaling = DXGI_SCALING_NONE;
        desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        desc1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        // Describe fullscreen swapchain
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc;
        fullscreen_desc.RefreshRate.Numerator = 60;
        fullscreen_desc.RefreshRate.Denominator = 1;
        fullscreen_desc.ScanlineOrdering = 
                DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        fullscreen_desc.Scaling = 
                DXGI_MODE_SCALING_UNSPECIFIED;
        fullscreen_desc.Windowed = TRUE;

        HRESULT result;

        // Create a DXGI factory
        result = CreateDXGIFactory2(0, &IID_IDXGIFactory5, 
                &swp_chain_info->factory5);
        show_error_if_failed(result);

        // Create swapchain
        IDXGISwapChain1 *swapchain1;
        result = swp_chain_info->factory5->lpVtbl->CreateSwapChainForHwnd(
                swp_chain_info->factory5,
                (IUnknown *) cmd_queue_info->cmd_queue, wnd_info->hwnd, 
                &desc1, &fullscreen_desc, NULL, &swapchain1);
        show_error_if_failed(result);

        result = swapchain1->lpVtbl->QueryInterface(swapchain1,
                &IID_IDXGISwapChain4, &swp_chain_info->swapchain4);
        show_error_if_failed(result);

        swapchain1->lpVtbl->Release(swapchain1);
}

UINT get_backbuffer_index(struct swapchain_info *swp_chain_info)
{
        return swp_chain_info->swapchain4->lpVtbl->GetCurrentBackBufferIndex(
                swp_chain_info->swapchain4);
}

ID3D12Resource *get_swapchain_buffer(struct swapchain_info *swp_chain_info,
                                    UINT buffer_index)
{
        ID3D12Resource *buffer;

        HRESULT result;

        result = swp_chain_info->swapchain4->lpVtbl->GetBuffer(
                swp_chain_info->swapchain4, buffer_index, &IID_ID3D12Resource,
                &buffer);
        show_error_if_failed(result);

        return buffer;
}

void present_swapchain(struct swapchain_info *swp_chain_info)
{
        HRESULT result;

        result = swp_chain_info->swapchain4->lpVtbl->Present(
                swp_chain_info->swapchain4, 0, 0);
        show_error_if_failed(result);
}

void release_swapchain(struct swapchain_info *swp_chain_info)
{
        // Release swapchain
        swp_chain_info->swapchain4->lpVtbl->Release(
                swp_chain_info->swapchain4);

        // Release DXGI factory
        swp_chain_info->factory5->lpVtbl->Release(swp_chain_info->factory5);
}