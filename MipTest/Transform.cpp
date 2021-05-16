#include "Transform.h"

Transform::Transform(Graphics &gfx, const DrawFunction &parent)
   :
   m_gfx(gfx),
   m_device(gfx.getDevice()),
   m_commandList(gfx.getCommandList()),
   m_parentTransform(parent)
{
}

void Transform::Bind(Graphics &gfx) noexcept
{
   //const auto modelView = m_parentTransform.getTransformXM() * gfx.get
}

void Transform::setIndices(int index, UINT start, UINT count)
{
}
