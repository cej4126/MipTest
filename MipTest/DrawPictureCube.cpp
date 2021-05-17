#include "DrawPictureCube.h"
#include "Shape.h"
#include "Texture.h"
#include "Transform.h"

DrawPictureCube::DrawPictureCube(Graphics &gfx, int &index, const std::string &fileName)
{
   std::size_t pos = fileName.find_last_of("/\\");
   std::string tag = "pictureCube#" + fileName.substr(pos + 1);

   std::vector<Shape::VertexTexture> vertices;
   std::vector<unsigned short>indices;
   Shape shape;
   shape.getCubeData(vertices, indices);

   std::shared_ptr<Object> object = Object::resolve(gfx, tag);
   if (!object->isInitialized())
   {
      object->setInitialized();

      object->loadVerticsBuffer(vertices);
      object->loadIndicesBuffer(indices);
      object->createShader(L"TextureVS.cso", L"TexturePS.cso");
      object->createRootSignature();
      object->createPipeLineState(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

   }
   addBind(std::move(object));

   std::shared_ptr<Texture> texture = Texture::resolve(gfx, tag);
   if (!texture->isInitialized())
   {
      texture->setInitialized();
      texture->createTexture(fileName, 0, 1);
   }
   addBind(std::move(texture));

   std::shared_ptr < Transform > transform = std::make_shared<Transform>(gfx, *this);
   UINT start = 0;
   UINT count = indices.size();
   transform->setIndices(index, start, count);
   ++index;

   addBind(std::move(transform));
}

void DrawPictureCube::update(float deltaTime) noexcept
{
}

XMMATRIX DrawPictureCube::getTransformXM() const noexcept
{
   return DirectX::XMMatrixScaling(m_size, m_size, m_size) *
      DirectX::XMMatrixRotationRollPitchYaw(m_rot.x, m_rot.y, m_rot.z) *
      DirectX::XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);
}

void DrawPictureCube::setPos(DirectX::XMFLOAT3 pos) noexcept
{
   m_pos = pos;
}
