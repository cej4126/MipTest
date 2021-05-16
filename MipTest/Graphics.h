#pragma once

#include "stdafx.h"
#include "Graphics.h"

class Graphics
{
public:
   Graphics(HWND hWnd, int width, int height);
   Graphics(const Graphics &) = delete;
   Graphics &operator=(const Graphics &) = delete;
   ~Graphics();

   struct TransformMatrix
   {
      XMMATRIX modelViewProj;
      XMMATRIX model;
   };

   TransformMatrix matrixBuffer = {};
   int ConstantBufferPerObjectAlignedSize = (sizeof(matrixBuffer) + 255) & ~255;

   ID3D12Device *getDevice() noexcept { return m_device.Get(); }
   ID3D12GraphicsCommandList *getCommandList() noexcept { return m_commandList.Get(); }

   void runCommandList();
   void onRenderBegin();
   void onRender();
   void drawCommandList();
   void onRenderEnd();
   void cleanUp();

   void setCamera(FXMMATRIX cameraTransfrom) noexcept { m_cameraTransform = cameraTransfrom; }
   XMMATRIX getCamera() const noexcept { return m_cameraTransform; }
   void setProjection(FXMMATRIX projection) noexcept { m_projection = projection; }
   XMMATRIX getProjection() const noexcept { return m_projection; }
   void createMatrixConstant(UINT count);
   void setMatrixConstant(UINT index, TransformMatrix matrix, int rootVS, int rootPS) noexcept;

   UINT64 UpdateSubresource(
      _In_ ID3D12Resource *pDestinationResource,
      _In_ ID3D12Resource *pIntermediate,
      _In_reads_(NumSubresources) D3D12_SUBRESOURCE_DATA *pSrcData);


protected:
   static const UINT m_bufferCount = 3;
   HWND m_hWnd;
   int m_width;
   int m_height;
   UINT m_frameIndex;
   int m_rtvDescriptorSize;
   UINT64 m_fenceValues[m_bufferCount];
   HANDLE m_fenceHandle;
   XMMATRIX m_projection;
   XMMATRIX m_cameraTransform;

   D3D12_VIEWPORT m_viewPort;
   D3D12_RECT m_scissorRect;

   ComPtr <IDXGIFactory4> m_dxgiFactory4;
   //ComPtr <IDXGIAdapter3> m_adapter;
   ComPtr <ID3D12Device> m_device;
   ComPtr <ID3D12CommandQueue>m_comandQueue;
   ComPtr <IDXGISwapChain3> m_swapChain;
   ComPtr <IDXGISwapChain1> m_swapChain1;
   ComPtr <ID3D12Resource> m_swapChainBuffers[m_bufferCount];
   ComPtr <ID3D12DescriptorHeap> m_rtvHeap;
   std::vector<ComPtr <ID3D12CommandAllocator>> m_commandAllocators;
   ComPtr <ID3D12GraphicsCommandList> m_commandList;
   ComPtr <ID3D12Fence> m_fences[m_bufferCount];
   ComPtr < ID3D12DescriptorHeap> m_dsDescriptorHeap;
   ComPtr <ID3D12Resource> m_depthStencilBuffer;

   // Direct X11
   ComPtr <ID3D11Device> m_x11Device;
   ComPtr <ID3D11DeviceContext> m_x11Context;
   ComPtr <ID3D11On12Device> m_x11Onx12Device;
   ComPtr <ID3D11Resource> m_x11WrappedBackBuffers[m_bufferCount];
   ComPtr <ID3D11RenderTargetView> m_x11Targets[m_bufferCount];
   D3D11_VIEWPORT m_x11ViewPort;
   ComPtr <ID3D11DepthStencilState> m_x11DepthStencelState;

private:
   void loadDevice();
   void loadBase();
   void loadDepent();
   void CreateFence();
   void waitForPreviousFrame();

   // DirectX11
   void loadX11OnX12Base();
   void onRenderX11();

   float m_aspectRatio;
};

