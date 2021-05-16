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

private:
   Bind::Bindable *object = nullptr;
};

