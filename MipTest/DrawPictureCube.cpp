#include "DrawPictureCube.h"
#include "Shape.h"

DrawPictureCube::DrawPictureCube(Graphics &gfx, int &index, const std::string &fileName)
{
   std::size_t pos = fileName.find_last_of("/\\");
   std::string tag = "pictureCube#" + fileName.substr(pos + 1);

   std::shared_ptr<Object> object = Object::resolve(gfx, tag);
   if (!object->isInitialized())
   {
      object->setInitialized();

      //std::shared_ptr<Object>
      std::vector<Shape::VertexTexture> vertices;
      std::vector<unsigned short>indices;

      Shape shape;
      shape.getCubeData(vertices, indices);

      object->loadVerticsBuffer(vertices);
      object->loadIndicesBuffer(indices);
      object->createShader(L"TextureVS.cso", L"TexturePS.cso");
      object->createRootSignature();
      object->createPipeLineState(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

   }
   addBind(std::move(object));


}

void DrawPictureCube::update(float deltaTime) noexcept
{
}

XMMATRIX DrawPictureCube::getTransformXM() const noexcept
{
   return XMMATRIX();
}
