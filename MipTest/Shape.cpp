#include "Shape.h"

void Shape::getCubeData(std::vector<VertexTexture> &vertices, std::vector<unsigned short> &indices)
{
   // Y
   // |
   //-Z - X

   vertices.reserve(4*6);
   // Front
   vertices.emplace_back(VertexTexture(-1.0f,  1.0f, -1.0f,  0.0f, 0.0f)); //  0
   vertices.emplace_back(VertexTexture( 1.0f,  1.0f, -1.0f,  1.0f, 0.0f)); //  1
   vertices.emplace_back(VertexTexture( 1.0f, -1.0f, -1.0f,  1.0f, 1.0f)); //  2
   vertices.emplace_back(VertexTexture(-1.0f, -1.0f, -1.0f,  0.0f, 1.0f)); //  3

   // Top
   vertices.emplace_back(VertexTexture(-1.0f,  1.0f,  1.0f, 0.0f, 0.0f)); //  4
   vertices.emplace_back(VertexTexture( 1.0f,  1.0f,  1.0f, 1.0f, 0.0f)); //  5
   vertices.emplace_back(VertexTexture( 1.0f,  1.0f, -1.0f, 1.0f, 1.0f)); //  6
   vertices.emplace_back(VertexTexture(-1.0f,  1.0f, -1.0f, 0.0f, 1.0f)); //  7

   // Back
   vertices.emplace_back(VertexTexture( 1.0f,  1.0f,  1.0f, 0.0f, 0.0f)); //  8
   vertices.emplace_back(VertexTexture(-1.0f,  1.0f,  1.0f, 1.0f, 0.0f)); //  9
   vertices.emplace_back(VertexTexture(-1.0f, -1.0f,  1.0f, 1.0f, 1.0f)); // 10
   vertices.emplace_back(VertexTexture( 1.0f, -1.0f,  1.0f, 0.0f, 1.0f)); // 11

   // Bottom
   vertices.emplace_back(VertexTexture(-1.0f, -1.0f, -1.0f, 0.0f, 0.0f)); // 12
   vertices.emplace_back(VertexTexture( 1.0f, -1.0f, -1.0f, 1.0f, 0.0f)); // 13
   vertices.emplace_back(VertexTexture( 1.0f, -1.0f,  1.0f, 1.0f, 1.0f)); // 14
   vertices.emplace_back(VertexTexture(-1.0f, -1.0f,  1.0f, 0.0f, 1.0f)); // 15

   // Right
   vertices.emplace_back(VertexTexture( 1.0f,  1.0f, -1.0f, 0.0f, 0.0f)); // 16
   vertices.emplace_back(VertexTexture( 1.0f,  1.0f,  1.0f, 1.0f, 0.0f)); // 17
   vertices.emplace_back(VertexTexture( 1.0f, -1.0f,  1.0f, 1.0f, 1.0f)); // 18
   vertices.emplace_back(VertexTexture( 1.0f, -1.0f, -1.0f, 0.0f, 1.0f)); // 19

   // Left
   vertices.emplace_back(VertexTexture(-1.0f,  1.0f,  1.0f, 0.0f, 0.0f)); // 20
   vertices.emplace_back(VertexTexture(-1.0f,  1.0f, -1.0f, 1.0f, 0.0f)); // 21
   vertices.emplace_back(VertexTexture(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f)); // 22
   vertices.emplace_back(VertexTexture(-1.0f, -1.0f,  1.0f, 0.0f, 1.0f)); // 23

   const unsigned short cubeIndices[]
   {
       0, 2, 3,  0, 1, 2,  // Front
       4, 6, 7,  4, 5, 6,  // Top
       8,10,11,  8, 9,10,  // Back
      12,14,15, 12,13,14,  // Bottom
      16,18,19, 16,17,18,  // Right
      20,22,23, 20,21,22   // Left
   };
   int count = _countof(cubeIndices);
   indices.reserve(count);
   for (int i = 0; i < count; i++)
   {
      indices.emplace_back(cubeIndices[i]);
   }
}
