#pragma once
#include "stdafx.h"
#include "Graphics.h"
#include "DrawFunction.h"
#include "Object.h"


class DrawPictureCube: public DrawFunction
{
public:
   DrawPictureCube(Graphics &gfx, int &index, const std::string &fileName);
   void update(float deltaTime) noexcept override;
   XMMATRIX getTransformXM() const noexcept override;
   void setPos(DirectX::XMFLOAT3 pos) noexcept;

private:
   Bind::Bindable *object = nullptr;
   DirectX::XMFLOAT3 m_pos = { 0.0f, 0.0f, 0.0f };
   DirectX::XMFLOAT3 m_rot = { 0.0f, 0.0f, 0.0f };
   float m_size = 1.0f;
};

