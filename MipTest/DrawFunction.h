#pragma once
#include "stdafx.h"
#include "Graphics.h"
#include "Bindable.h"

class DrawFunction
{
public:
   DrawFunction() = default;
   DrawFunction(const DrawFunction &) = delete;
   virtual ~DrawFunction() = default;

   void draw() const noexcept;
   virtual void update(float deltaTime) noexcept {};
   virtual XMMATRIX getTransformXM() const noexcept = 0;
   virtual void setPos(DirectX::XMFLOAT3 pos) noexcept {};

protected:
   void addBind(std::shared_ptr<Bind::Bindable> bind) noexcept;

private:
   std::vector<std::shared_ptr<Bind::Bindable>> binds;
};

template<class R>
std::vector<std::shared_ptr<Bind::Bindable>> DrawBase;

