#include "Transform.hlsli"
ConstantBuffer<TransformType> transform : register(b0);

struct VS_INPUT
{
   float4 position : POSITION;
   float2 texCoord: TEXCOORD;
};

struct VS_OUTPUT
{
   float4 position: SV_POSITION;
   float2 texCoord: TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
   VS_OUTPUT output;
   output.position = mul(input.position, transform.modelView);
   output.texCoord = input.texCoord;
   return output;
}

