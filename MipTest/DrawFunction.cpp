#include "DrawFunction.h"

void DrawFunction::draw(Graphics &gfx) const noexcept
{
   for (auto &b : binds)
   {
      b->Bind(gfx);
   }
}

void DrawFunction::addBind(std::shared_ptr<Bind::Bindable> bind) noexcept
{
   binds.push_back(std::move(bind));
}
