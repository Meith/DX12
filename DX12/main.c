#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_5.h>
#include <d3dcompiler.h>

#include <assert.h>
#include <stdio.h>

#include "linmath.h"

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);

int CALLBACK WinMain(HINSTANCE h_instance, HINSTANCE hprev_instance, LPSTR lp_cmd_line, int n_cmd_show)
{
        // Fill in window class struct
        WNDCLASSEX wc;
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = h_instance;
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(h_instance, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = "DX12WindowClass";
        wc.hIconSm = NULL;

        // Register window class struct
        RegisterClassEx(&wc);

        // Create the window
        HWND hwnd = CreateWindowEx(0, wc.lpszClassName, "DX12Window", WS_OVERLAPPEDWINDOW, 100, 50, 800, 600, NULL, NULL, h_instance, NULL);
        assert(hwnd);

        HRESULT result;

        // Enable debug layer
        #if defined(_DEBUG)
        ID3D12Debug2 *debug2;
        result = D3D12GetDebugInterface(&IID_ID3D12Debug2, &debug2);
        assert(SUCCEEDED(result));
        debug2->lpVtbl->SetGPUBasedValidationFlags(debug2, D3D12_GPU_BASED_VALIDATION_FLAGS_NONE);
        #endif

        // Create D3D12 device
        ID3D12Device4 *device4;
        result = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device4, &device4);
        assert(SUCCEEDED(result));

        // Describe command queue
        D3D12_COMMAND_QUEUE_DESC queue_desc;
        queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queue_desc.Priority = 0;
        queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queue_desc.NodeMask = 0;

        // Create queue
        ID3D12CommandQueue *command_queue;
        result = device4->lpVtbl->CreateCommandQueue(device4, &queue_desc, &IID_ID3D12CommandQueue, &command_queue);
        assert(SUCCEEDED(result));

        #define BUFFER_COUNT 2

        // Describe swapchain
        DXGI_SWAP_CHAIN_DESC1 swapchain_desc1;
        swapchain_desc1.Width = 0;
        swapchain_desc1.Height = 0;
        swapchain_desc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchain_desc1.Stereo = FALSE;
        swapchain_desc1.SampleDesc.Count = 1;
        swapchain_desc1.SampleDesc.Quality = 0;
        swapchain_desc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_desc1.BufferCount = BUFFER_COUNT;
        swapchain_desc1.Scaling = DXGI_SCALING_NONE;
        swapchain_desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapchain_desc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapchain_desc1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        // Describe fullscreen swapchain
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapchain_fullscreen_desc;
        swapchain_fullscreen_desc.RefreshRate.Numerator = 60;
        swapchain_fullscreen_desc.RefreshRate.Denominator = 1;
        swapchain_fullscreen_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swapchain_fullscreen_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        swapchain_fullscreen_desc.Windowed = TRUE;

        // Create a DXGI factory
        IDXGIFactory5 *factory5;
        result = CreateDXGIFactory2(
                #if defined (_DEBUG)
                DXGI_CREATE_FACTORY_DEBUG,
                #else
                0,
                #endif
                &IID_IDXGIFactory5,
                &factory5
                );
        assert(SUCCEEDED(result));

        // Create swapchain
        IDXGISwapChain4 *swapchain4;
        result = factory5->lpVtbl->CreateSwapChainForHwnd(factory5, (IUnknown *) command_queue, hwnd, &swapchain_desc1, &swapchain_fullscreen_desc, NULL, (IDXGISwapChain1 **) &swapchain4);
        assert(SUCCEEDED(result));

        // Create render target descriptor heap description
        D3D12_DESCRIPTOR_HEAP_DESC rt_descriptor_heap_desc;
        rt_descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rt_descriptor_heap_desc.NumDescriptors = BUFFER_COUNT;
        rt_descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rt_descriptor_heap_desc.NodeMask = 0;

        // Create render target descriptor heap
        ID3D12DescriptorHeap *rt_descriptor_heap;
        result = device4->lpVtbl->CreateDescriptorHeap(device4, &rt_descriptor_heap_desc, &IID_ID3D12DescriptorHeap, &rt_descriptor_heap);

        // Now that we have space, lets create render targets
        // We have two buffers in the swapchain, each will need a render target view
        // First we need to get the size of a single render target descriptor
        UINT rt_descriptor_heap_size = device4->lpVtbl->GetDescriptorHandleIncrementSize(device4, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // Now we need to get the starting position of the render target descriptor in its heap
        // We alias this pointer with a CPU side handle
        D3D12_CPU_DESCRIPTOR_HANDLE rt_cpu_handle;
        rt_descriptor_heap->lpVtbl->GetCPUDescriptorHandleForHeapStart(rt_descriptor_heap, &rt_cpu_handle);

        // This will store the swapchain buffer resources
        // For consistency we'll name them render target buffers
        ID3D12Resource *rt_buffers[BUFFER_COUNT];

        // Now we can create the render target views
        // We iterate over the buffers, ie. get the resource
        // Point our cpu handle pointer to the correct location in the heap
        // And then call create
        for (UINT i = 0; i < BUFFER_COUNT; ++i) {
                // First get the buffer for which we are creating a render target view for
                // This buffer can be stored on the heap as a general read/write resource
                result = swapchain4->lpVtbl->GetBuffer(swapchain4, i, &IID_ID3D12Resource, &(rt_buffers[i]));
                assert(SUCCEEDED(result));

                // Because we have two buffers, and we need to create two render target views, we need to make sure we are pointing at the correct position in the heap
                rt_cpu_handle.ptr += i * rt_descriptor_heap_size;

                // Now we can create the render target view for the buffer we got, and store it at the pointer in the heap
                device4->lpVtbl->CreateRenderTargetView(device4, rt_buffers[i], NULL, rt_cpu_handle);
        }  

        // Create command allocator
        // This allocates space for command lists
        // Command lists that will consists of commands we will be sending to the GPU to do the work
        // We will create one command allocator per frame
        // So that we can record commands for one frame while we wait for the other to get executed on the GPU
        ID3D12CommandAllocator *command_allocator[BUFFER_COUNT];
        for (UINT i = 0; i < BUFFER_COUNT; ++i) {
                result = device4->lpVtbl->CreateCommandAllocator(device4, D3D12_COMMAND_LIST_TYPE_DIRECT, &IID_ID3D12CommandAllocator, &command_allocator[i]);
                assert(SUCCEEDED(result));
        }

        // Lets get our current back buffer for setting frame related commands
        UINT back_buffer_index = swapchain4->lpVtbl->GetCurrentBackBufferIndex(swapchain4);

        // Create command lists
        ID3D12GraphicsCommandList3 *graphics_command_list3;
        result = device4->lpVtbl->CreateCommandList(device4, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator[back_buffer_index], NULL, &IID_ID3D12GraphicsCommandList3, &graphics_command_list3);
        assert(SUCCEEDED(result));

        // Command lists are created in record mode by default
        // We need to close them
        graphics_command_list3->lpVtbl->Close(graphics_command_list3);

        // Create fence for cpu/gpu synchronization
        // We create one fence value per frame for the same reasons as command allocator
        ID3D12Fence *fence;
        UINT64 fence_value[BUFFER_COUNT] = { 0 };
        result = device4->lpVtbl->CreateFence(device4, fence_value[back_buffer_index], D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, &fence);
        assert(SUCCEEDED(result));

        // Create event handle for stalling cpu thread
        HANDLE fence_event;
        fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
        assert(fence_event);
 
        // Lets start setting up traingle data
        #define VERT_COUNT 3 

        struct vertex_input {
                vec3 position;
                vec3 colour;
        };

        struct vertex_input triangle_verts[VERT_COUNT];
        
        // Bottom left vertex
        triangle_verts[0].position[0] = -1.0f;
        triangle_verts[0].position[1] = -1.0f;
        triangle_verts[0].position[2] = 0.0f;
        triangle_verts[0].colour[0] = 1.0f;
        triangle_verts[0].colour[1] = 0.0f;
        triangle_verts[0].colour[2] = 0.0f;

        // Top vertex
        triangle_verts[1].position[0] = 0.0f;
        triangle_verts[1].position[1] = 1.0f;
        triangle_verts[1].position[2] = 0.0f;
        triangle_verts[1].colour[0] = 0.0f;
        triangle_verts[1].colour[1] = 1.0f;
        triangle_verts[1].colour[2] = 0.0f;

        // Bottom right vertex
        triangle_verts[2].position[0] = 1.0f;
        triangle_verts[2].position[1] = -1.0f;
        triangle_verts[2].position[2] = 0.0f;
        triangle_verts[2].colour[0] = 0.0f;
        triangle_verts[2].colour[1] = 0.0f;
        triangle_verts[2].colour[2] = 1.0f;

        // We need to create some space (a heap) to store the triangle data
        // We can create different types of heap as they serve different purposes
        // One type is used to upload data from the CPU to the GPU, one is used for reading data back from the GPU to the CPU and the last one for the GPU to use for it's computations
        // This heap is going to be used by the Input Assembly stage and the vertex shader so it's type needs to be the third type, called "default"
        D3D12_HEAP_PROPERTIES vert_buffer_heap_properties;
        vert_buffer_heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
        vert_buffer_heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        vert_buffer_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        vert_buffer_heap_properties.CreationNodeMask = 0;
        vert_buffer_heap_properties.VisibleNodeMask = 0;

        // Now we need to describe the data we want to upload to the heap
        // The total size of data, type of data like buffer or texture
        D3D12_RESOURCE_DESC vert_buffer_resc_desc;
        vert_buffer_resc_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        vert_buffer_resc_desc.Alignment = 0;
        vert_buffer_resc_desc.Width = sizeof(triangle_verts);
        vert_buffer_resc_desc.Height = 1;
        vert_buffer_resc_desc.DepthOrArraySize = 1;
        vert_buffer_resc_desc.MipLevels = 1;
        vert_buffer_resc_desc.Format = DXGI_FORMAT_UNKNOWN;
        vert_buffer_resc_desc.SampleDesc.Count = 1;
        vert_buffer_resc_desc.SampleDesc.Quality = 0;
        vert_buffer_resc_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        vert_buffer_resc_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // Now we can create the vertex buffer resource
        // Usually we need to create the heap that we described before the resource
        // However, this one time use resource is created using CreateCommittedResource
        // This call creates the heap big enough for the resource and the resource itself
        ID3D12Resource *vert_buffer;
        result = device4->lpVtbl->CreateCommittedResource(device4, &vert_buffer_heap_properties, D3D12_HEAP_FLAG_NONE, &vert_buffer_resc_desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, &IID_ID3D12Resource, &vert_buffer);
        assert(SUCCEEDED(result));

        // However we cannot directly upload our triangle vertex data to this heap and resource
        // Well, we could if we used an upload heap type
        // Upload heap type has CPU access, but does not have enough bandwidth for transferring data to the GPU
        // The default heap has maximum bandwidth to transfer data to the GPU
        // So the ideal solution is to upload data via the upload heap and then copy that data to the default heap so that it can be passed to the GPU
        // So lets create our upload heap type resource
        // We set it up the same way by first setting up heap properties
        D3D12_HEAP_PROPERTIES vert_buffer_upload_heap_properties;
        vert_buffer_upload_heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
        vert_buffer_upload_heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        vert_buffer_upload_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        vert_buffer_upload_heap_properties.CreationNodeMask = 0;
        vert_buffer_upload_heap_properties.VisibleNodeMask = 0;

        // Then we describe the resource
        D3D12_RESOURCE_DESC vert_buffer_upload_resc_desc;
        vert_buffer_upload_resc_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        vert_buffer_upload_resc_desc.Alignment = 0;
        vert_buffer_upload_resc_desc.Width = sizeof(triangle_verts);
        vert_buffer_upload_resc_desc.Height = 1;
        vert_buffer_upload_resc_desc.DepthOrArraySize = 1;
        vert_buffer_upload_resc_desc.MipLevels = 1;
        vert_buffer_upload_resc_desc.Format = DXGI_FORMAT_UNKNOWN;
        vert_buffer_upload_resc_desc.SampleDesc.Count = 1;
        vert_buffer_upload_resc_desc.SampleDesc.Quality = 0;
        vert_buffer_upload_resc_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        vert_buffer_upload_resc_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // Lastly we can create the upload resource
        ID3D12Resource *vert_buffer_upload;
        result = device4->lpVtbl->CreateCommittedResource(device4, &vert_buffer_upload_heap_properties, D3D12_HEAP_FLAG_NONE, &vert_buffer_upload_resc_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, &IID_ID3D12Resource, &vert_buffer_upload);
        assert(SUCCEEDED(result));

        // Now we can copy the data on the vertex upload buffer 
        void *vertex_data;
        vert_buffer_upload->lpVtbl->Map(vert_buffer_upload, 0, NULL, &vertex_data);
        memcpy(vertex_data, triangle_verts, sizeof(triangle_verts));
        vert_buffer_upload->lpVtbl->Unmap(vert_buffer_upload, 0, NULL);

        // Finally we can copy data from the vertex upload buffer to the vertex buffer
        // Before that, we need to reset and open the command list and allocator
        for (UINT i = 0; i < BUFFER_COUNT; ++i) {
                command_allocator[i]->lpVtbl->Reset(command_allocator[i]);
        }

        graphics_command_list3->lpVtbl->Reset(graphics_command_list3, command_allocator[back_buffer_index], NULL);

        // Now we can record the command to copy the data
        graphics_command_list3->lpVtbl->CopyBufferRegion(graphics_command_list3, vert_buffer, 0, vert_buffer_upload, 0, sizeof(triangle_verts));

        // Lastly we need to transition the resource of the vertex buffer from a copy destination to vertex and constant buffer
        D3D12_RESOURCE_BARRIER vert_buffer_resource_barrier;
        vert_buffer_resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        vert_buffer_resource_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        vert_buffer_resource_barrier.Transition.pResource = vert_buffer;
        vert_buffer_resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        vert_buffer_resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        vert_buffer_resource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

        graphics_command_list3->lpVtbl->ResourceBarrier(graphics_command_list3, 1, &vert_buffer_resource_barrier);

        // Once the resource buffer is created, we need to create a view object for it
        // A view tells the GPU, where it resides in the heap and how to read/interpret it
        D3D12_VERTEX_BUFFER_VIEW vert_buffer_view;
        vert_buffer_view.BufferLocation = vert_buffer->lpVtbl->GetGPUVirtualAddress(vert_buffer);
        vert_buffer_view.SizeInBytes = sizeof(triangle_verts);
        vert_buffer_view.StrideInBytes = sizeof(struct vertex_input);

        // Lets do the same for the index buffer
        // Lets start by creating index data
        #define INDEX_COUNT 3
        WORD triangle_indices[INDEX_COUNT];
        triangle_indices[0] = 0;
        triangle_indices[1] = 1;
        triangle_indices[2] = 2;

        // Lets create heap properties for indices heap
        D3D12_HEAP_PROPERTIES index_buffer_heap_properties;
        index_buffer_heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
        index_buffer_heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        index_buffer_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        index_buffer_heap_properties.CreationNodeMask = 0;
        index_buffer_heap_properties.VisibleNodeMask = 0;       

        // Lets describe index buffer resource
        D3D12_RESOURCE_DESC index_buffer_resc_desc;
        index_buffer_resc_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        index_buffer_resc_desc.Alignment = 0;
        index_buffer_resc_desc.Width = sizeof(triangle_indices);
        index_buffer_resc_desc.Height = 1;
        index_buffer_resc_desc.DepthOrArraySize = 1;
        index_buffer_resc_desc.MipLevels = 1;
        index_buffer_resc_desc.Format = DXGI_FORMAT_UNKNOWN;
        index_buffer_resc_desc.SampleDesc.Count = 1;
        index_buffer_resc_desc.SampleDesc.Quality = 0;
        index_buffer_resc_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        index_buffer_resc_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // Finally, lets create the index buffer resource and it's heap
        ID3D12Resource *index_buffer;
        result = device4->lpVtbl->CreateCommittedResource(device4, &index_buffer_heap_properties, D3D12_HEAP_FLAG_NONE, &index_buffer_resc_desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, &IID_ID3D12Resource, &index_buffer);
        assert(SUCCEEDED(result));

        D3D12_HEAP_PROPERTIES index_buffer_upload_heap_properties;
        index_buffer_upload_heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
        index_buffer_upload_heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        index_buffer_upload_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        index_buffer_upload_heap_properties.CreationNodeMask = 0;
        index_buffer_upload_heap_properties.VisibleNodeMask = 0;

        // Then we describe the resource
        D3D12_RESOURCE_DESC index_buffer_upload_resc_desc;
        index_buffer_upload_resc_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        index_buffer_upload_resc_desc.Alignment = 0;
        index_buffer_upload_resc_desc.Width = sizeof(triangle_indices);
        index_buffer_upload_resc_desc.Height = 1;
        index_buffer_upload_resc_desc.DepthOrArraySize = 1;
        index_buffer_upload_resc_desc.MipLevels = 1;
        index_buffer_upload_resc_desc.Format = DXGI_FORMAT_UNKNOWN;
        index_buffer_upload_resc_desc.SampleDesc.Count = 1;
        index_buffer_upload_resc_desc.SampleDesc.Quality = 0;
        index_buffer_upload_resc_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        index_buffer_upload_resc_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // Lastly we can create the upload resource
        ID3D12Resource *index_buffer_upload;
        result = device4->lpVtbl->CreateCommittedResource(device4, &index_buffer_upload_heap_properties, D3D12_HEAP_FLAG_NONE, &index_buffer_upload_resc_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, &IID_ID3D12Resource, &index_buffer_upload);
        assert(SUCCEEDED(result));

        // Now we can copy the data on the index upload buffer 
        void *index_data;
        index_buffer_upload->lpVtbl->Map(index_buffer_upload, 0, NULL, &index_data);
        memcpy(index_data, triangle_indices, sizeof(triangle_indices));
        index_buffer_upload->lpVtbl->Unmap(index_buffer_upload, 0, NULL);

        // Finally we can copy data from the index upload buffer to the vertex buffer
        graphics_command_list3->lpVtbl->CopyBufferRegion(graphics_command_list3, index_buffer, 0, index_buffer_upload, 0, sizeof(triangle_indices));

        // Lastly we need to transition the resource of the index buffer from a copy destination to index buffer
        D3D12_RESOURCE_BARRIER index_buffer_resource_barrier;
        index_buffer_resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        index_buffer_resource_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        index_buffer_resource_barrier.Transition.pResource = index_buffer;
        index_buffer_resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        index_buffer_resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        index_buffer_resource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;

        graphics_command_list3->lpVtbl->ResourceBarrier(graphics_command_list3, 1, &index_buffer_resource_barrier);

        // And lastly a view for the index buffer
        D3D12_INDEX_BUFFER_VIEW index_buffer_view;
        index_buffer_view.BufferLocation = index_buffer->lpVtbl->GetGPUVirtualAddress(index_buffer);
        index_buffer_view.SizeInBytes = sizeof(triangle_indices);
        index_buffer_view.Format = DXGI_FORMAT_R16_UINT;

        // Now we need to create a heap for depth stencil buffer
        // Like all other heap creation steps, we first describe it
        D3D12_DESCRIPTOR_HEAP_DESC ds_descriptor_heap_desc;
        ds_descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        ds_descriptor_heap_desc.NumDescriptors = 1;
        ds_descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ds_descriptor_heap_desc.NodeMask = 0;

        // Create heap for depth stencil buffer
        ID3D12DescriptorHeap *ds_heap;
        result = device4->lpVtbl->CreateDescriptorHeap(device4, &ds_descriptor_heap_desc, &IID_ID3D12DescriptorHeap, &ds_heap);
        assert(SUCCEEDED(result));

        // Create the depth stencil buffer resource clear value
        D3D12_CLEAR_VALUE ds_clear_value;
        ds_clear_value.Format = DXGI_FORMAT_D32_FLOAT;
        ds_clear_value.DepthStencil.Depth = 1.0f;
        ds_clear_value.DepthStencil.Stencil = 0;

        // Create the depth stencil buffer heap properties
        D3D12_HEAP_PROPERTIES ds_heap_properties;
        ds_heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
        ds_heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        ds_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        ds_heap_properties.CreationNodeMask = 0;
        ds_heap_properties.VisibleNodeMask = 0;

        // Create the depth stencil buffer resource description
        D3D12_RESOURCE_DESC ds_resource_desc;
        ds_resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        ds_resource_desc.Alignment = 0;
        ds_resource_desc.Width = 800;
        ds_resource_desc.Height = 600;
        ds_resource_desc.DepthOrArraySize = 1;
        ds_resource_desc.MipLevels = 0;
        ds_resource_desc.Format = DXGI_FORMAT_D32_FLOAT;
        ds_resource_desc.SampleDesc.Count = 1;
        ds_resource_desc.SampleDesc.Quality = 0;
        ds_resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        ds_resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        // Now we can create the committed resource for depth stencil buffer
        ID3D12Resource *ds_buffer;
        result = device4->lpVtbl->CreateCommittedResource(device4, &ds_heap_properties, D3D12_HEAP_FLAG_NONE, &ds_resource_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &ds_clear_value, &IID_ID3D12Resource, &ds_buffer);
        assert(SUCCEEDED(result));

        // Create depth stencil buffer view description
        D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc;
        dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
        dsv_desc.Texture2D.MipSlice = 0;

        // Get CPU handle for depth stencil buffer
        D3D12_CPU_DESCRIPTOR_HANDLE ds_cpu_handle;
        ds_heap->lpVtbl->GetCPUDescriptorHandleForHeapStart(ds_heap, &ds_cpu_handle);
        
        // Create depth stencil buffer view
        device4->lpVtbl->CreateDepthStencilView(device4, ds_buffer, &dsv_desc, ds_cpu_handle);

        // Time to load the shaders
        ID3DBlob *shader_error_blob = NULL;
        ID3DBlob *vert_shader_blob;
        result = D3DCompileFromFile(L"shaders\\tri_vert_shader.hlsl", NULL, NULL, "main", "vs_5_1", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0, &vert_shader_blob, &shader_error_blob);
        assert(SUCCEEDED(result));
        assert(shader_error_blob == NULL);

        ID3DBlob *pix_shader_blob = NULL;
        result = D3DCompileFromFile(L"shaders\\tri_pix_shader.hlsl", NULL, NULL, "main", "ps_5_1", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0, &pix_shader_blob, &shader_error_blob);
        assert(SUCCEEDED(result));
        assert(shader_error_blob == NULL);

        // Lets setup the input layout
        // This is so that GPU can know how to interpret the data being passed to the vertex shader
        // We have two inputs for the vertex shader: position and colour
        #define VERT_ATT_COUNT 2
        D3D12_INPUT_ELEMENT_DESC input_element_desc[VERT_ATT_COUNT];
        input_element_desc[0].SemanticName = "POSITION";
        input_element_desc[0].SemanticIndex = 0;
        input_element_desc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        input_element_desc[0].InputSlot = 0;
        input_element_desc[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        input_element_desc[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        input_element_desc[0].InstanceDataStepRate = 0;

        input_element_desc[1].SemanticName = "COLOR";
        input_element_desc[1].SemanticIndex = 0;
        input_element_desc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        input_element_desc[1].InputSlot = 0;
        input_element_desc[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        input_element_desc[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        input_element_desc[1].InstanceDataStepRate = 0;

        // Time to create the root signature
        // A root signature is a function signature for all the parameters we need to pass to the shaders
        // For now, we only have one constant buffer which is a 4x4 mvp matrix
        // In reality, we could have textures, buffers, samplers
        // The function signature could be parameters of different types 
        // Can be constants without resource buffers
        // Resource buffers directly stored in the signature without a heap
        // Or finally resource buffers stored in the heap accessed through indexing

        // Lets start with defining root signature flags
        // This determines which shader stage gets parameters passed to them
        // In our case it's only the vertex shader so we deny the rest
        D3D12_ROOT_SIGNATURE_FLAGS root_sig_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                                                    D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                                                    D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                                                    D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
                                                    D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        // Now lets add the parameters we want to pass using the root signature
        // In our case it's just one 32 bit constant paramater the mvp matrix
        D3D12_ROOT_PARAMETER1 root_parameter1;
        root_parameter1.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        root_parameter1.Constants.ShaderRegister = 0;
        root_parameter1.Constants.RegisterSpace = 0;
        root_parameter1.Constants.Num32BitValues = sizeof(mat4x4) / sizeof(float);
        root_parameter1.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        // Now we need to describe the root signature we want to create
        D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc;
        root_sig_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        root_sig_desc.Desc_1_1.NumParameters = 1;
        root_sig_desc.Desc_1_1.pParameters = &root_parameter1;
        root_sig_desc.Desc_1_1.NumStaticSamplers = 0;
        root_sig_desc.Desc_1_1.pStaticSamplers = NULL;
        root_sig_desc.Desc_1_1.Flags = root_sig_flags;

        // Before we create the root signature, there's one additional step
        // This step is an exception and only there for root signatures
        // We need to serialize the description into a binary object and then create it
        // This allows for offline root signature creation and therefore providing some opportunities for optimization
        ID3D10Blob *root_sig_blob;
        ID3D10Blob *root_sig_error_blob = NULL;
        result = D3D12SerializeVersionedRootSignature(&root_sig_desc, &root_sig_blob, &root_sig_error_blob);
        assert(SUCCEEDED(result));
        assert(root_sig_error_blob == NULL);

        // Now we can create the root signature
        ID3D12RootSignature *root_sig;
        result = device4->lpVtbl->CreateRootSignature(device4, 0, root_sig_blob->lpVtbl->GetBufferPointer(root_sig_blob), root_sig_blob->lpVtbl->GetBufferSize(root_sig_blob), &IID_ID3D12RootSignature, &root_sig);
        assert(SUCCEEDED(result));

        // The last and final step for the setup of the graphics pipeline is to create a pipeline state object
        // Through the pipeline state object, we tell the GPU which parts of the pipeline we are going to be customizing
        // For the rest, the GPU is free to use the default setup
        // For our case, we are going to be setting up the root signature, input layout, primitive topology, vertex and pixel shader, depth stencil buffer and render target format
        // Lets start by describing the pipeline state object
        D3D12_GRAPHICS_PIPELINE_STATE_DESC graphics_pso_desc;
        graphics_pso_desc.pRootSignature = root_sig;
        graphics_pso_desc.VS.pShaderBytecode = vert_shader_blob->lpVtbl->GetBufferPointer(vert_shader_blob);
        graphics_pso_desc.VS.BytecodeLength = vert_shader_blob->lpVtbl->GetBufferSize(vert_shader_blob);
        graphics_pso_desc.PS.pShaderBytecode = pix_shader_blob->lpVtbl->GetBufferPointer(pix_shader_blob);
        graphics_pso_desc.PS.BytecodeLength = pix_shader_blob->lpVtbl->GetBufferSize(pix_shader_blob);
        graphics_pso_desc.DS.pShaderBytecode = NULL;
        graphics_pso_desc.DS.BytecodeLength = 0;
        graphics_pso_desc.HS.pShaderBytecode = NULL;
        graphics_pso_desc.HS.BytecodeLength = 0;
        graphics_pso_desc.GS.pShaderBytecode = NULL;
        graphics_pso_desc.GS.BytecodeLength = 0;
        graphics_pso_desc.StreamOutput.pSODeclaration = NULL;
        graphics_pso_desc.StreamOutput.NumEntries = 0;
        graphics_pso_desc.StreamOutput.pBufferStrides = NULL;
        graphics_pso_desc.StreamOutput.NumStrides = 0;
        graphics_pso_desc.StreamOutput.RasterizedStream = 0;
        graphics_pso_desc.BlendState.AlphaToCoverageEnable = FALSE;
        graphics_pso_desc.BlendState.IndependentBlendEnable = FALSE;
        graphics_pso_desc.BlendState.RenderTarget[0].BlendEnable = FALSE;
        graphics_pso_desc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
        graphics_pso_desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        graphics_pso_desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
        graphics_pso_desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        graphics_pso_desc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        graphics_pso_desc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        graphics_pso_desc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        graphics_pso_desc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
        graphics_pso_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        graphics_pso_desc.SampleMask = UINT_MAX;
        graphics_pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        graphics_pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        graphics_pso_desc.RasterizerState.FrontCounterClockwise = FALSE;
        graphics_pso_desc.RasterizerState.DepthBias = 0;
        graphics_pso_desc.RasterizerState.DepthBiasClamp = 0.0f;
        graphics_pso_desc.RasterizerState.SlopeScaledDepthBias = 0.0f;
        graphics_pso_desc.RasterizerState.DepthClipEnable = TRUE;
        graphics_pso_desc.RasterizerState.MultisampleEnable = FALSE;
        graphics_pso_desc.RasterizerState.AntialiasedLineEnable = FALSE;
        graphics_pso_desc.RasterizerState.ForcedSampleCount = 0;
        graphics_pso_desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        graphics_pso_desc.DepthStencilState.DepthEnable = TRUE;
        graphics_pso_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        graphics_pso_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        graphics_pso_desc.DepthStencilState.StencilEnable = FALSE;
        graphics_pso_desc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        graphics_pso_desc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        graphics_pso_desc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        graphics_pso_desc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        graphics_pso_desc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        graphics_pso_desc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        graphics_pso_desc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        graphics_pso_desc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        graphics_pso_desc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        graphics_pso_desc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        graphics_pso_desc.InputLayout.pInputElementDescs = input_element_desc;
        graphics_pso_desc.InputLayout.NumElements = VERT_ATT_COUNT;
        graphics_pso_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
        graphics_pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        graphics_pso_desc.NumRenderTargets = 1;
        graphics_pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        for (UINT i = 1; i < 8; ++i) {
                graphics_pso_desc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
        }
        graphics_pso_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        graphics_pso_desc.SampleDesc.Count = 1;
        graphics_pso_desc.SampleDesc.Quality = 0;
        graphics_pso_desc.NodeMask = 0;
        graphics_pso_desc.CachedPSO.pCachedBlob = NULL;
        graphics_pso_desc.CachedPSO.CachedBlobSizeInBytes = 0;
        graphics_pso_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        // Finally create the pipeline state object
        ID3D12PipelineState *graphics_pso;
        result = device4->lpVtbl->CreateGraphicsPipelineState(device4, &graphics_pso_desc, &IID_ID3D12PipelineState, &graphics_pso);
        assert(SUCCEEDED(result));

        // Now we need to create a view port for the screen we are going to be drawing to
        D3D12_VIEWPORT view_port;
        view_port.TopLeftX = 0.0f;
        view_port.TopLeftY = 0.0f;
        view_port.Width = 800.0f;
        view_port.Height = 600.0f;
        view_port.MinDepth = D3D12_MIN_DEPTH;
        view_port.MaxDepth = D3D12_MAX_DEPTH;

        // Create the scissor rectangle for the pixel operations
        D3D12_RECT scissor_rect;
        scissor_rect.left = 0;
        scissor_rect.top = 0;
        scissor_rect.right = LONG_MAX;
        scissor_rect.bottom = LONG_MAX;

        // Show window
        ShowWindow(hwnd, n_cmd_show);

        // Message and render loop
        MSG msg;
        PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
        while (msg.message != WM_QUIT) {
                // Run message loop
                if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                }

                // Update constant per frame data
                mat4x4 mvp;
                mvp[0][0] = 1.0f; mvp[0][1] = 0.0f; mvp[0][2] = 0.0f; mvp[0][3] = 0.0f;
                mvp[1][0] = 0.0f; mvp[1][1] = 1.0f; mvp[1][2] = 0.0f; mvp[1][3] = 0.0f;
                mvp[2][0] = 0.0f; mvp[2][1] = 0.0f; mvp[2][2] = 1.0f; mvp[2][3] = 0.0f;
                mvp[3][0] = 0.0f; mvp[3][1] = 0.0f; mvp[3][2] = 0.0f; mvp[3][3] = 1.0f;

                // We want to clear our render target view
                // In order to do that, we need to first draw/write to our render target resource, and then present/read from it
                // So the same resource, needs to be used for two different purposes
                // We need to correctly transition between these two states using resource barriers
                // Let's first change render target view resource to render target state by creating a resource barrier for it
                D3D12_RESOURCE_BARRIER rt_res_barrier;
                rt_res_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                rt_res_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                rt_res_barrier.Transition.pResource = rt_buffers[back_buffer_index];
                rt_res_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                rt_res_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
                rt_res_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

                // The command list now needs to make sure it does the correct transition before it can pass a command to clear screen
                graphics_command_list3->lpVtbl->ResourceBarrier(graphics_command_list3, 1, &rt_res_barrier);

                // Now we can get the CPU pointer for the render target descriptor we created for this buffer
                // To do that lets first get the CPU handle pointing to the start of the render target descriptor in it's respective heap
                rt_descriptor_heap->lpVtbl->GetCPUDescriptorHandleForHeapStart(rt_descriptor_heap, &rt_cpu_handle);

                // Now lets offset the pointer to the position of the render target descriptor used for the current back buffer
                rt_cpu_handle.ptr += back_buffer_index * rt_descriptor_heap_size;

                // Finally, we can now clear the colour of the correct render target descriptor who is in the correct render target state
                float clear_color[] = { 0.4f, 0.6f, 0.9f, 1.0f };
                graphics_command_list3->lpVtbl->ClearRenderTargetView(graphics_command_list3, rt_cpu_handle, clear_color, 0, NULL);

                // Now we want to record the command to clear the depth stencil buffer
                graphics_command_list3->lpVtbl->ClearDepthStencilView(graphics_command_list3, ds_cpu_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);

                // Now we want to set the graphics pipeline state object
                graphics_command_list3->lpVtbl->SetPipelineState(graphics_command_list3, graphics_pso);

                // Now we want to set the render targets
                graphics_command_list3->lpVtbl->OMSetRenderTargets(graphics_command_list3, 1, &rt_cpu_handle, FALSE, &ds_cpu_handle);

                // Now we set viewport
                graphics_command_list3->lpVtbl->RSSetViewports(graphics_command_list3, 1, &view_port);

                // Now we set scissor rect
                graphics_command_list3->lpVtbl->RSSetScissorRects(graphics_command_list3, 1, &scissor_rect);

                // Now lets define the primitive topology for the command list
                graphics_command_list3->lpVtbl->IASetPrimitiveTopology(graphics_command_list3, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                // Now we want to set the root signature
                graphics_command_list3->lpVtbl->SetGraphicsRootSignature(graphics_command_list3, root_sig);

                // Now we want to set the constants in the root signature
                graphics_command_list3->lpVtbl->SetGraphicsRoot32BitConstants(graphics_command_list3, 0, sizeof(mat4x4) / sizeof(float), mvp, 0);

                // Now we can set the vertex buffer
                graphics_command_list3->lpVtbl->IASetVertexBuffers(graphics_command_list3, 0, 1, &vert_buffer_view);

                // Now we can set the index buffer
                graphics_command_list3->lpVtbl->IASetIndexBuffer(graphics_command_list3, &index_buffer_view);

                // Finally we have everything we need to draw the triangle set up
                // Lets call the draw command
                graphics_command_list3->lpVtbl->DrawIndexedInstanced(graphics_command_list3, INDEX_COUNT, 1, 0, 0, 0);

                // We are now ready to present
                // However, we haven't told the GPU to transition our render target view to the correct presentable state
                // So lets create a resource barrier for that
                D3D12_RESOURCE_BARRIER present_res_barrier;
                present_res_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                present_res_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                present_res_barrier.Transition.pResource = rt_buffers[back_buffer_index];
                present_res_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                present_res_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
                present_res_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

                // Now lets record this in the command list just like before
                graphics_command_list3->lpVtbl->ResourceBarrier(graphics_command_list3, 1, &present_res_barrier);

                // Close command list for execution
                result = graphics_command_list3->lpVtbl->Close(graphics_command_list3);
                assert(SUCCEEDED(result));

                // Finally we have recorded all the commands we want to execute and the respective transitions required
                // We can now execute the command list
                // We have created a graphics command list, so first we need to store it in an array of general command list
                // This can store multiple graphics command lists that need to be executed on the GPU
                ID3D12CommandList *command_lists[] = { (ID3D12CommandList *)graphics_command_list3 };
                command_queue->lpVtbl->ExecuteCommandLists(command_queue, _countof(command_lists), command_lists);

                // Present swapchain after all GPU work is done
                // As long as we are using the same command queue for executing the command list and the swapchain, we don't have to worry about synchronizing it
                swapchain4->lpVtbl->Present(swapchain4, 0, 0);

                // Once on it's way to execute on the GPU, we need to synchronize to make sure we don't start recording commands on the command list for this frame's command allocator
                // Remember we have two, so the idea is to record the other frame while the current one executes on the GPU
                // In the event the other frame is also done and sent of the GPU, and the first is yet to be completed, we wait on the CPU side
                // In order to do that, we first find out the fence value for the current buffer and signal the GPU
                UINT64 current_fence_value = fence_value[back_buffer_index];
                result = command_queue->lpVtbl->Signal(command_queue, fence, current_fence_value);
                assert(SUCCEEDED(result));

                // Then we get the next frame's buffer index
                back_buffer_index = swapchain4->lpVtbl->GetCurrentBackBufferIndex(swapchain4);

                // Wait for fence object to reach updated fence_value on the CPU signaled from the GPU for the new frame
                // If the new frame is all clear to go, we don't need to wait, otherwise we do and we go into this if block to wait on the CPU side
                if (fence->lpVtbl->GetCompletedValue(fence) < fence_value[back_buffer_index]) {
                        // If it's not reached that value yet on the GPU, set the event that needs to be fired when that value is reached
                        result = fence->lpVtbl->SetEventOnCompletion(fence, fence_value[back_buffer_index], fence_event);
                        assert(SUCCEEDED(result));
                        // On the CPU, we wait for that event to be fired
                        WaitForSingleObject(fence_event, INFINITE);

                }

                // Increment fence value to be set by current back buffer index by 1 so that GPU never gets signaled with the same fence value by two back buffers 
                fence_value[back_buffer_index] = current_fence_value + 1;

                // Once the gpu is done executing we want to reset our command allocator and command list so that we can re use them
                result = command_allocator[back_buffer_index]->lpVtbl->Reset(command_allocator[back_buffer_index]);
                assert(SUCCEEDED(result));

                result = graphics_command_list3->lpVtbl->Reset(graphics_command_list3, command_allocator[back_buffer_index], NULL);
                assert(SUCCEEDED(result));
        }

        // Updates the fence object to 'fence_value' *on the GPU*
        // It can take a while depending on how busy the GPU is
        // The idea is that we will wait on the fence object to reach the fence_value before we start releasing resources on the CPU
        // Don't confuse this line to behave like it's already setting fence object to 'fence_value'
        // It's not, fence object's value is still what you initialised it with
        result = command_queue->lpVtbl->Signal(command_queue, fence, fence_value[back_buffer_index]);
        assert(SUCCEEDED(result));

        // Wait for fence object to reach updated fence_value on the CPU signaled from the GPU
        if (fence->lpVtbl->GetCompletedValue(fence) < fence_value[back_buffer_index]) {
                // If it's not reached that value yet on the GPU, set the event that needs to be fired when that value is reached
                result = fence->lpVtbl->SetEventOnCompletion(fence, fence_value[back_buffer_index], fence_event);
                assert(SUCCEEDED(result));
                // On the CPU, we wait for that event to be fired
                WaitForSingleObject(fence_event, INFINITE);
        }

        // Release grahics pipeline state object
        graphics_pso->lpVtbl->Release(graphics_pso);

        // Release root signature
        root_sig->lpVtbl->Release(root_sig);

        // Release root signature blob
        root_sig_blob->lpVtbl->Release(root_sig_blob);

        // Release pixel shader blob
        pix_shader_blob->lpVtbl->Release(pix_shader_blob);

        // Release vertex shader blob
        vert_shader_blob->lpVtbl->Release(vert_shader_blob);

        // Release depth stencil buffer resource
        ds_buffer->lpVtbl->Release(ds_buffer);

        // Release depth stencl buffer heap
        ds_heap->lpVtbl->Release(ds_heap);

        // Release index buffer upload resource
        index_buffer_upload->lpVtbl->Release(index_buffer_upload);

        // Release index buffer resource
        index_buffer->lpVtbl->Release(index_buffer);

        // Release vertex buffer upload resource
        vert_buffer_upload->lpVtbl->Release(vert_buffer_upload);

        // Release vertex buffer resource
        vert_buffer->lpVtbl->Release(vert_buffer);

        // Release event
        CloseHandle(fence_event);

        // Release fence
        fence->lpVtbl->Release(fence);

        // Release command list
        graphics_command_list3->lpVtbl->Release(graphics_command_list3);

        // Release command allocator
        for (UINT i = 0; i < BUFFER_COUNT; ++i) {
                command_allocator[i]->lpVtbl->Release(command_allocator[i]);
        }

        // Release the render target view buffers
        for (UINT i = 0; i < BUFFER_COUNT; ++i) {
                rt_buffers[i]->lpVtbl->Release(rt_buffers[i]);
        }

        // Release render target view descriptor heap
        rt_descriptor_heap->lpVtbl->Release(rt_descriptor_heap);

        // Release swapchain
        swapchain4->lpVtbl->Release(swapchain4);

        // Release DXGI factory
        factory5->lpVtbl->Release(factory5);

        // Release D3D12 command queue
        command_queue->lpVtbl->Release(command_queue);
 
        // Release D3D12 device
        device4->lpVtbl->Release(device4);

        // Release debug layer
        #if defined(_DEBUG)
        debug2->lpVtbl->Release(debug2);
        #endif

        DestroyWindow(hwnd);

        UnregisterClass(wc.lpszClassName, wc.hInstance);

        return (int) msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
        switch (msg)
        {
                case WM_DESTROY:
                        PostQuitMessage(0);
                        return 0;

                default:
                        return DefWindowProc(hwnd, msg, w_param, l_param);
        }
}
