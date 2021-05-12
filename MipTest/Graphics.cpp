#include "Graphics.h"

Graphics::Graphics(HWND hWnd, int width, int height)
   :
   m_hWnd(hWnd),
   m_width(width),
   m_height(height)
{
   m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);

#if defined (_DEBUG)
   ComPtr <ID3D12Debug> m_debugInterface;
   ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&m_debugInterface)));
   m_debugInterface->EnableDebugLayer();
#endif

   loadDevice();
   loadBase();
   loadX11OnX12Base();
}

Graphics::~Graphics()
{
}

void Graphics::runCommandList()
{
}

void Graphics::onRenderBegin()
{
}

void Graphics::onRender()
{
}

void Graphics::drawCommandList()
{
}

void Graphics::onRenderEnd()
{
}

void Graphics::cleanUp()
{
}

void Graphics::createMatrixConstant(UINT count)
{
}

void Graphics::setMatrixConstant(UINT index, TransformMatrix matrix, int rootVS, int rootPS) noexcept
{
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

   // Create Command Queue
   D3D12_COMMAND_QUEUE_DESC queueDesc;
   ZeroMemory(&queueDesc, sizeof(queueDesc));
   queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
   queueDesc.NodeMask = 0;
   queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
   queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

   ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_comandQueue.ReleaseAndGetAddressOf())));

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
   swapChainDesc.BufferCount = m_bufferCount;
   swapChainDesc.Scaling = DXGI_SCALING_NONE;
   swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
   swapChainDesc.Flags = 0;
   ThrowIfFailed(m_dxgiFactory4->CreateSwapChainForHwnd(m_comandQueue.Get(),
      m_hWnd,
      &swapChainDesc,
      nullptr,
      nullptr,
      m_swapChain1.ReleaseAndGetAddressOf()));

   ThrowIfFailed(m_swapChain1.As(&m_swapChain));

   // Get Swap Chain Buffers
   for (UINT i = 0; i < m_bufferCount; i++)
   {
      ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_swapChainBuffer[i].ReleaseAndGetAddressOf())));
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
   heapDescriptor.NumDescriptors = m_bufferCount;
   heapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

   ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDescriptor, IID_PPV_ARGS(m_rtvHeap.ReleaseAndGetAddressOf())));

   m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
   for (UINT i = 0; i < m_bufferCount; i++)
   {
      D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
      rtvHandle.ptr += (UINT64)i * (UINT64)m_rtvDescriptorSize;
      m_device->CreateRenderTargetView(m_swapChainBuffer[i].Get(), nullptr, rtvHandle);
   }

   // Create Command Allocators
   for (UINT i = 0; i < m_bufferCount; i++)
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

   CreateFence();
}

void Graphics::CreateFence()
{
   // Create Fence
   for (UINT i = 0; i < m_bufferCount; i++)
   {
      ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence[i].ReleaseAndGetAddressOf())));
      m_fenceValue[i] = 0;
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
      reinterpret_cast<IUnknown **>(m_comandQueue.GetAddressOf()),
      1, 0, &m_x11Device, &m_x11Context, nullptr));

   // Query the X11OnX12 device from the X11 device
   ThrowIfFailed(m_x11Device.As(&m_x11Onx12Device));

   // Create RTV
   for (UINT i = 0; i < m_bufferCount; i++)
   {
      D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
      ThrowIfFailed(m_x11Onx12Device->CreateWrappedResource(
         m_swapChainBuffer[i].Get(),
         &d3d11Flags,
         D3D12_RESOURCE_STATE_RENDER_TARGET,
         D3D12_RESOURCE_STATE_PRESENT,
         IID_PPV_ARGS(&x11WrappedBackBuffer[i])));

      ThrowIfFailed(m_x11Device->CreateRenderTargetView(x11WrappedBackBuffer[i].Get(), nullptr, &m_x11Target[i]));
   }

   m_x11ViewPort.Width = (float)m_width;
   m_x11ViewPort.Height = (float)m_height;
   m_x11ViewPort.MaxDepth = 1;
   m_x11ViewPort.MinDepth = 0;
   m_x11ViewPort.TopLeftX = 0;
   m_x11ViewPort.TopLeftY = 0;

   // Depth Stencil
   D3D11_DEPTH_STENCIL_DESC depthDescription = {};
   depthDescription.StencilEnable = false;
   depthDescription.DepthEnable = false;

   ThrowIfFailed(m_x11Device->CreateDepthStencilState(
      &depthDescription, &m_x11DepthStencelState));
}
