#include "Graphics.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

const std::wstring UTF8ToWideString(const std::string &str)
{
   wchar_t wstr[MAX_PATH];
   if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str.c_str(), -1, wstr, MAX_PATH))
      wstr[0] = L'\0';
   return wstr;
}

Graphics::Graphics(HWND hWnd, int width, int height)
   :
   m_hWnd(hWnd),
   m_width(width),
   m_height(height)
{
   // debug 3d
#if defined (_DEBUG)
   ComPtr <ID3D12Debug> m_debugInterface;
   ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&m_debugInterface)));
   m_debugInterface->EnableDebugLayer();
#endif

   loadDevice();
   loadBase();
   loadX11OnX12Base();
   loadBase2D();

   // ImGui
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO &io = ImGui::GetIO(); //(void)io;
   io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
   ImGui::StyleColorsDark();
   ImGui_ImplWin32_Init(hWnd);
   ImGui_ImplDX11_Init(m_x11Device.Get(), m_x11DeviceContext.Get());
}

Graphics::~Graphics()
{
   ImGui_ImplDX11_Shutdown();
}

void Graphics::runCommandList()
{
   // Run the initial command list
   m_commandList->Close();
   ID3D12CommandList *ppCommandLists[] = { m_commandList.Get() };
   m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

   m_fenceValues[m_frameIndex]++;
   ThrowIfFailed(m_commandQueue->Signal(m_fences[m_frameIndex].Get(), m_fenceValues[m_frameIndex]));
}

void Graphics::waitForPreviousFrame()
{
   // Wait for the last frame to complete
   if (m_fences[m_frameIndex]->GetCompletedValue() < m_fenceValues[m_frameIndex])
   {
      ThrowIfFailed(m_fences[m_frameIndex]->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceHandle));
      WaitForSingleObject(m_fenceHandle, INFINITE);
   }

   m_fenceValues[m_frameIndex]++;
}

void Graphics::onRenderX11()
{
   m_x11DeviceContext->OMSetDepthStencilState(m_x11DepthStencilState.Get(), 0u);
   m_x11DeviceContext->OMSetRenderTargets(1u, m_x11Targets[m_frameIndex].GetAddressOf(), nullptr);
   m_x11DeviceContext->RSSetViewports(1u, &m_x11ViewPort);
}

void Graphics::loadBase2D()
{
   float dpiX;
   float dpiY;

   dpiX = (float)GetDpiForWindow(m_hWnd);
   dpiY = dpiX;
   D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
      D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
      D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
      dpiX,
      dpiY
   );

   D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
   ThrowIfFailed(m_d2dFactory->CreateDevice(m_dxgiDevice.Get(), &m_d2dDevice));
   ThrowIfFailed(m_d2dDevice->CreateDeviceContext(deviceOptions, &m_x11d2dDeviceContext));
   ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &m_dWriteFactory));
   for (UINT i{ 0 }; i < Buffer_Count; i++)
   {
      // Create a render target for D2D to draw directly to this back buffer.
      ComPtr<IDXGISurface> surface;
      ThrowIfFailed(m_x11WrappedBackBuffers[i].As(&surface));
      ThrowIfFailed(m_x11d2dDeviceContext->CreateBitmapFromDxgiSurface(
         surface.Get(),
         &bitmapProperties,
         &m_x11d2dRenderTargets[i]));
   }
}

void Graphics::onRenderBegin()
{
   m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
   waitForPreviousFrame();

   ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());

   ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr));

   D3D12_RESOURCE_BARRIER resourceBarrier;
   resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
   resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
   resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
   resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
   resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
   resourceBarrier.Transition.pResource = m_swapChainBuffers[m_frameIndex].Get();
   m_commandList->ResourceBarrier(1, &resourceBarrier);

   // Indicate that the back buffer will be used as a render target.
   D3D12_CPU_DESCRIPTOR_HANDLE renderTargetDesc = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
   renderTargetDesc.ptr += (SIZE_T)m_frameIndex * (SIZE_T)m_rtvDescriptorSize;

   D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
   m_commandList->OMSetRenderTargets(1, &renderTargetDesc, FALSE, &dsvHandle);

   // Record commands.
   const float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
   m_commandList->ClearRenderTargetView(renderTargetDesc, clearColor, 0, nullptr);
   m_commandList->ClearDepthStencilView(m_dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

   // Set necessary state.
   m_commandList->RSSetViewports(1, &m_viewPort);
   m_commandList->RSSetScissorRects(1, &m_scissorRect);
}

void Graphics::onRender()
{
   m_x11On12Device->AcquireWrappedResources(m_x11WrappedBackBuffers[m_frameIndex].GetAddressOf(), 1);

   // d2Write
   // Render text directly to the back buffer.
   m_x11d2dDeviceContext->SetTarget(m_x11d2dRenderTargets[m_frameIndex].Get());

   onRenderX11();
}

void Graphics::drawCommandList()
{
   m_commandList->Close();

   // Run the command list
   ID3D12CommandList *ppCommandLists[] = { m_commandList.Get() };
   m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void Graphics::onRenderEnd()
{
   m_x11On12Device->ReleaseWrappedResources(m_x11WrappedBackBuffers[m_frameIndex].GetAddressOf(), 1);

   m_x11DeviceContext->Flush();
   ThrowIfFailed(m_commandQueue->Signal(m_fences[m_frameIndex].Get(), m_fenceValues[m_frameIndex]));

   // Present the frame.
   ThrowIfFailed(m_swapChain->Present(1, 0));
}

void Graphics::cleanUp()
{
   // wait for the gpu to finish all frames
   for (int i = 0; i < Buffer_Count; ++i)
   {
      m_frameIndex = i;
      waitForPreviousFrame();
   }

   // Wait for windows to finish
   for (int i = 0; i < Buffer_Count; ++i)
   {
      m_frameIndex = i;
      ThrowIfFailed(m_commandQueue->Signal(m_fences[m_frameIndex].Get(), m_fenceValues[m_frameIndex]));
      waitForPreviousFrame();
   }
}

void Graphics::createMatrixConstant(UINT index)
{
   int ConstantBufferPerObjectAlignedSize = (sizeof(XMMATRIX) + 255) & ~255;

   // Matrix Constant buffer
   D3D12_HEAP_PROPERTIES constantHeapUpload = {};
   constantHeapUpload.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
   constantHeapUpload.CreationNodeMask = 1;
   constantHeapUpload.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
   constantHeapUpload.Type = D3D12_HEAP_TYPE_UPLOAD;
   constantHeapUpload.VisibleNodeMask = 1;

   D3D12_RESOURCE_DESC constantHeapDesc = {};
   constantHeapDesc.Alignment = 0;
   constantHeapDesc.DepthOrArraySize = 1;
   constantHeapDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
   constantHeapDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
   constantHeapDesc.Format = DXGI_FORMAT_UNKNOWN;
   constantHeapDesc.Height = 1;
   constantHeapDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
   constantHeapDesc.SampleDesc.Count = 1;
   constantHeapDesc.SampleDesc.Quality = 0;
   constantHeapDesc.Width = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
   constantHeapDesc.MipLevels = 1;

   ThrowIfFailed(m_device->CreateCommittedResource(
      &constantHeapUpload,
      D3D12_HEAP_FLAG_NONE,
      &constantHeapDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&m_matrixBufferUploadHeaps)));

   D3D12_RANGE readRange;
   readRange.Begin = 0;
   readRange.End = 0;
   ThrowIfFailed(m_matrixBufferUploadHeaps->Map(0, &readRange, reinterpret_cast<void **>(&m_matrixBufferGPUAddress)));
}

void Graphics::setMatrixConstant(UINT index, TransformMatrix matrix, int rootVS, int rootPS) noexcept
{
   int ConstantBufferPerObjectAlignedSize = (sizeof(matrix) + 255) & ~255;

   if (rootVS >= 0)
   {
      m_commandList->SetGraphicsRootConstantBufferView(rootVS,
         m_matrixBufferUploadHeaps->GetGPUVirtualAddress() + index * ConstantBufferPerObjectAlignedSize);
   }

   if (rootPS >= 0)
   {
      m_commandList->SetGraphicsRootConstantBufferView(rootPS,
         m_matrixBufferUploadHeaps->GetGPUVirtualAddress() + index * ConstantBufferPerObjectAlignedSize);
   }

   memcpy(m_matrixBufferGPUAddress + index * ConstantBufferPerObjectAlignedSize, &matrix, sizeof(matrix));
}

void Graphics::loadDevice()
{
   UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
   dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

   ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory4)));
   int adapterIndex = 0;
   bool adapterFound = false;
   ComPtr<IDXGIAdapter1> adapterTemp;
   HRESULT hr;

   while (m_dxgiFactory4->EnumAdapters1(adapterIndex, adapterTemp.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND)
   {
      DXGI_ADAPTER_DESC1 description1;
      adapterTemp->GetDesc1(&description1);
      if (description1.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)
      {
         continue;
      }

      hr = D3D12CreateDevice(adapterTemp.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
      if (SUCCEEDED(hr))
      {
         adapterFound = true;
         break;
      }
      ++adapterIndex;
   }

   if (!adapterFound)
   {
      throw;
   }

   // device
   hr = D3D12CreateDevice(adapterTemp.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
   ThrowIfFailed(hr);

   // Create Command Queue
   D3D12_COMMAND_QUEUE_DESC queueDesc;
   ZeroMemory(&queueDesc, sizeof(queueDesc));
   queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
   queueDesc.NodeMask = 0;
   queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
   queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

   ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_commandQueue.ReleaseAndGetAddressOf())));

   // Create Swap Chains
   DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
   ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
   swapChainDesc.Width = m_width;
   swapChainDesc.Height = m_height;
   swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
   swapChainDesc.Stereo = FALSE;
   swapChainDesc.SampleDesc.Count = 1;
   swapChainDesc.SampleDesc.Quality = 0;
   swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
   swapChainDesc.BufferCount = Buffer_Count;
   swapChainDesc.Scaling = DXGI_SCALING_NONE;
   swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
   swapChainDesc.Flags = 0;
   ThrowIfFailed(m_dxgiFactory4->CreateSwapChainForHwnd(m_commandQueue.Get(),
      m_hWnd,
      &swapChainDesc,
      nullptr,
      nullptr,
      m_swapChain1.ReleaseAndGetAddressOf()));

   ThrowIfFailed(m_swapChain1.As(&m_swapChain));

   // Get Swap Chain Buffers
   for (UINT i = 0; i < Buffer_Count; i++)
   {
      ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_swapChainBuffers[i].ReleaseAndGetAddressOf())));
   }
   m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void Graphics::loadBase()
{
   // Create Descriptor Heap Render Target View (RTV)
   D3D12_DESCRIPTOR_HEAP_DESC heapDescriptor;
   ZeroMemory(&heapDescriptor, sizeof(heapDescriptor));
   heapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
   heapDescriptor.NodeMask = 0;
   heapDescriptor.NumDescriptors = Buffer_Count;
   heapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

   ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDescriptor, IID_PPV_ARGS(m_rtvHeap.ReleaseAndGetAddressOf())));

   m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
   for (UINT i = 0; i < Buffer_Count; i++)
   {
      D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
      rtvHandle.ptr += (UINT64)i * (UINT64)m_rtvDescriptorSize;
      m_device->CreateRenderTargetView(m_swapChainBuffers[i].Get(), nullptr, rtvHandle);
   }

   // Create Command Allocators
   for (UINT i = 0; i < Buffer_Count; i++)
   {
      ComPtr<ID3D12CommandAllocator> commandAllocator;
      ThrowIfFailed(m_device->CreateCommandAllocator(
         D3D12_COMMAND_LIST_TYPE_DIRECT,
         IID_PPV_ARGS(commandAllocator.ReleaseAndGetAddressOf())));
      m_commandAllocators.push_back(commandAllocator);
   }

   // Create Command List
   ThrowIfFailed(m_device->CreateCommandList(
      0,
      D3D12_COMMAND_LIST_TYPE_DIRECT,
      m_commandAllocators[0].Get(),
      nullptr, IID_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));

   createFence();

   loadDepent();

   m_viewPort.TopLeftX = 0.0f;
   m_viewPort.TopLeftY = 0.0f;
   m_viewPort.Width = (float)m_width;
   m_viewPort.Height = (float)m_height;
   m_viewPort.MinDepth = D3D12_MIN_DEPTH;
   m_viewPort.MaxDepth = D3D12_MAX_DEPTH;

   m_scissorRect.left = 0;
   m_scissorRect.top = 0;
   m_scissorRect.right = m_width;
   m_scissorRect.bottom = m_height;
}

void Graphics::loadDepent()
{
   // create a depth stencil descriptor heap
   D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
   dsvHeapDesc.NumDescriptors = 1;
   dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
   dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
   ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsDescriptorHeap)));

   D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
   depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
   depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
   depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

   // Create Depth Stencil Buffer
   D3D12_CLEAR_VALUE depthOptimizedClearValue;
   ZeroMemory(&depthOptimizedClearValue, sizeof(depthOptimizedClearValue));
   depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
   depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
   depthOptimizedClearValue.DepthStencil.Stencil = 0;

   D3D12_HEAP_PROPERTIES depthheapProps;
   ZeroMemory(&depthheapProps, sizeof(depthheapProps));
   depthheapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
   depthheapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
   depthheapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
   depthheapProps.CreationNodeMask = 1;
   depthheapProps.VisibleNodeMask = 1;

   D3D12_RESOURCE_DESC depthresourceDesc;
   ZeroMemory(&depthresourceDesc, sizeof(depthresourceDesc));
   depthresourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
   depthresourceDesc.Alignment = 0;
   depthresourceDesc.Width = m_width;
   depthresourceDesc.Height = m_height;
   depthresourceDesc.DepthOrArraySize = 1;
   depthresourceDesc.MipLevels = 0;
   depthresourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
   depthresourceDesc.SampleDesc.Count = 1;
   depthresourceDesc.SampleDesc.Quality = 0;
   depthresourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
   depthresourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

   ThrowIfFailed(m_device->CreateCommittedResource(
      &depthheapProps,
      D3D12_HEAP_FLAG_NONE,
      &depthresourceDesc,
      D3D12_RESOURCE_STATE_DEPTH_WRITE,
      &depthOptimizedClearValue,
      IID_PPV_ARGS(m_depthStencilBuffer.ReleaseAndGetAddressOf())));
   m_depthStencilBuffer->SetName(L"depth stencil buffer");

   m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &depthStencilDesc, m_dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

}

void Graphics::createFence()
{
   // Create Fence
   for (UINT i = 0; i < Buffer_Count; i++)
   {
      ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fences[i].ReleaseAndGetAddressOf())));
      m_fenceValues[i] = 0;
   }

   // Create Fence Event Handle
   m_fenceHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
   if (m_fenceHandle == NULL)
   {
      throw;
   }
}

void Graphics::loadX11OnX12Base()
{
   UINT x11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
   x11DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

   ThrowIfFailed(D3D11On12CreateDevice(m_device.Get(),
      x11DeviceFlags,
      nullptr, 0,
      reinterpret_cast<IUnknown **>(m_commandQueue.GetAddressOf()),
      1, 0, &m_x11Device, &m_x11DeviceContext, nullptr));

   // Query the X11OnX12 device from the X11 device
   ThrowIfFailed(m_x11Device.As(&m_x11On12Device));

   // DWrite
   D2D1_FACTORY_OPTIONS d2dFactoryOptions = {};

   ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, &m_d2dFactory));
   ThrowIfFailed(m_x11On12Device.As(&m_dxgiDevice));

   // Create RTV
   for (UINT i = 0; i < Buffer_Count; i++)
   {
      D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
      ThrowIfFailed(m_x11On12Device->CreateWrappedResource(
         m_swapChainBuffers[i].Get(),
         &d3d11Flags,
         D3D12_RESOURCE_STATE_RENDER_TARGET,
         D3D12_RESOURCE_STATE_PRESENT,
         IID_PPV_ARGS(&m_x11WrappedBackBuffers[i])));

      ThrowIfFailed(m_x11Device->CreateRenderTargetView(m_x11WrappedBackBuffers[i].Get(), nullptr, &m_x11Targets[i]));
   }

   m_x11ViewPort.Width = (float)m_width;
   m_x11ViewPort.Height = (float)m_height;
   m_x11ViewPort.MinDepth = 0.0f;
   m_x11ViewPort.MaxDepth = 1.0f;
   m_x11ViewPort.TopLeftX = 0;
   m_x11ViewPort.TopLeftY = 0;

   // Depth Stencil
   D3D11_DEPTH_STENCIL_DESC depthDescription = {};
   depthDescription.StencilEnable = false;
   depthDescription.DepthEnable = false;

   ThrowIfFailed(m_x11Device->CreateDepthStencilState(
      &depthDescription, &m_x11DepthStencilState));
}

