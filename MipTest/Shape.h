#pragma once
#include "stdafx.h"

class Shape
{
public:
   struct VertexTexture
   {
      XMFLOAT3 position;
      XMFLOAT2 texture;
      VertexTexture(float x, float y, float z, float u, float v)
      {
         position.x = x;
         position.y = y;
         position.z = z;
         texture.x = u;
         texture.y = v;
      }
   };

   void getCubeData(std::vector<VertexTexture> &vertices, std::vector<unsigned short>&indices);
};

