#include "Transform.h"

Transform::Transform(Graphics &gfx, const DrawFunction &parent, int rootVS, int rootPS)
   :
   m_gfx(gfx),
   m_device(gfx.getDevice()),
   m_commandList(gfx.getCommandList()),
   m_parentTransform(parent)
{
}

void Transform::Bind(Graphics &gfx) noexcept
{
   auto c = gfx.getCamera();
   auto p = m_parentTransform.getTransformXM();
   auto g = gfx.getProjection();

   const auto modelView = m_parentTransform.getTransformXM() * gfx.getCamera();
   const Graphics::TransformMatrix contantMatrix =
   {
      XMMatrixTranspose(modelView),
      XMMatrixTranspose(
      modelView *
      gfx.getProjection())
   };

   int index = getIndex();
   if (index == -1)
   {
      assert(false);
   }

   gfx.setMatrixConstant(index, contantMatrix, m_rootVS, m_rootPS);

   //int materialIndex = m_parentTransform.getMaterialIndex();
   //if (materialIndex != -1)
   //{
   //   gfx.SetMaterialConstant(materialIndex);
   //}

   if (m_indicesCount > 0)
   {
      m_commandList->DrawIndexedInstanced(m_indicesCount, 1u, m_indicesStart, 0u, 0u);
   }

   //if (lightBufferActive)
   //{
   //   auto dataCopy = gfx.lightData;
   //   const auto pos = XMLoadFloat3(&dataCopy.viewLightPos);
   //   XMMATRIX cam = gfx.GetCamera();
   //   XMStoreFloat3(&dataCopy.viewLightPos, XMVector3Transform(pos, cam));
   //   memcpy(lightBufferGPUAddress, &dataCopy, sizeof(dataCopy));
   //}
}

void Transform::setIndices(int index, UINT start, UINT count)
{
   setIndex(index);
   m_indicesStart = start;
   m_indicesCount = count;
}
