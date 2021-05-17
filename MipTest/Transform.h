#pragma once
#include "stdafx.h"
#include "Graphics.h"
#include "Bindable.h"
#include "DrawFunction.h"

class Transform : public Bind::Bindable
{
public:
   Transform(Graphics &gfx, const DrawFunction &parent, int rootVS = 0, int rootPS = -1);
   void Bind(Graphics &gfx) noexcept override;
   void setIndices(int index, UINT start, UINT count);

private:
   Graphics &m_gfx;
   ID3D12Device *m_device;
   ID3D12GraphicsCommandList *m_commandList;

   const DrawFunction &m_parentTransform;
   UINT m_indicesStart = 0;
   UINT m_indicesCount = 0;

   int m_rootVS = 0;
   int m_rootPS = -1;
};

