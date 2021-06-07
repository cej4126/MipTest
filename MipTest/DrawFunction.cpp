#include "DrawFunction.h"

void DrawFunction::draw() const noexcept
{
   for (auto &b : binds)
   {
      b->draw();
   }
}

void DrawFunction::addBind(std::shared_ptr<Bind::Bindable> bind) noexcept
{
   binds.push_back(std::move(bind));
}
