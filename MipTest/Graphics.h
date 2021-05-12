#pragma once

#include "stdafx.h"

using namespace Microsoft::WRL;
using namespace DirectX;

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

   void runCommandList();
   void onRenderBegin();
   void onRender();
   void drawCommandList();
   void onRenderEnd();
   void cleanUp();

   void createMatrixConstant(UINT count);
   void setMatrixConstant(UINT index, TransformMatrix matrix, int rootVS, int rootPS) noexcept;

protected:
   static const UINT m_bufferCount = 3;
   HWND m_hWnd;
   int m_width;
   int m_height;
   UINT m_frameIndex;
   int m_rtvDescriptorSize;
   UINT64 m_fenceValue[m_bufferCount];
   HANDLE m_fenceHandle;

   ComPtr <IDXGIFactory4> m_dxgiFactory4;
   ComPtr <ID3D12Device> m_device;
   ComPtr <ID3D12CommandQueue>m_comandQueue;
   ComPtr <IDXGISwapChain3> m_swapChain;
   ComPtr <IDXGISwapChain1> m_swapChain1;
   ComPtr <ID3D12Resource> m_swapChainBuffer[m_bufferCount];
   ComPtr <ID3D12DescriptorHeap> m_rtvHeap;
   std::vector<ComPtr <ID3D12CommandAllocator>> m_commandAllocators;
   ComPtr <ID3D12CommandList> m_commandList;
   ComPtr <ID3D12Fence> m_fence[m_bufferCount];

   // Direct X11
   ComPtr <ID3D11Device> m_x11Device;
   ComPtr <ID3D11DeviceContext> m_x11Context;
   ComPtr <ID3D11On12Device> m_x11Onx12Device;
   ComPtr <ID3D11Resource> x11WrappedBackBuffer[m_bufferCount];
   ComPtr <ID3D11RenderTargetView> m_x11Target[m_bufferCount];
   D3D11_VIEWPORT m_x11ViewPort;
   ComPtr <ID3D11DepthStencilState> m_x11DepthStencelState;

private:
   void loadDevice();
   void loadBase();
   void CreateFence();
   void loadX11OnX12Base();


   float m_aspectRatio;
};

