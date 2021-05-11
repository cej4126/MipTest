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
   HWND m_hWnd;
   int m_width;
   int m_height;

   ComPtr <IDXGIFactory4> m_dxgiFactory4;
   ComPtr <ID3D12Device> m_device;
   ComPtr <ID3D12CommandQueue>m_comandQueue;

private:
   void loadDevice();

   float m_aspectRatio;
};

