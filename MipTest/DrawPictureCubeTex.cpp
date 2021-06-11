#include "DrawPictureCubeTex.h"
#include "Shape.h"
#include "TextureMipmap.h"
#include "Transform.h"

DrawPictureCubeTex::DrawPictureCubeTex(Graphics &gfx, int &index, const std::string &fileName)
{
   std::size_t pos = fileName.find_last_of("/\\");
   std::string tag = "pictureCubeTex#" + fileName.substr(pos + 1);

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

   std::shared_ptr<TextureMipmap> texture = TextureMipmap::resolve(gfx, tag);
   if (!texture->isInitialized())
   {
      texture->setInitialized();

      std::wstring wpath = UTF8ToWideString(fileName);
      const wchar_t *wcs = wpath.c_str();
      texture->createTextureMipmap(wcs, 0, 1);
   }
   addBind(std::move(texture));

   std::shared_ptr < Transform > transform = std::make_shared<Transform>(gfx, *this);
   UINT start = 0;
   UINT count = (UINT)indices.size();
   transform->setIndices(index, start, count);
   ++index;

   addBind(std::move(transform));
}

void DrawPictureCubeTex::update(float deltaTime) noexcept
{
}

XMMATRIX DrawPictureCubeTex::getTransformXM() const noexcept
{
   return DirectX::XMMatrixScaling(m_size, m_size, m_size) *
      DirectX::XMMatrixRotationRollPitchYaw(m_rot.x, m_rot.y, m_rot.z) *
      DirectX::XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);
}

void DrawPictureCubeTex::setPos(DirectX::XMFLOAT3 pos) noexcept
{
   m_pos = pos;
}
