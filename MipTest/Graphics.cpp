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

}
