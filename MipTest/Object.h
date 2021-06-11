#pragma once
#include "stdafx.h"
#include "Bindable.h"
#include "Graphics.h"
#include "Shape.h"

class Object : public Bind::Bindable
{
public:
   Object(Graphics& gfx, std::string tag);

   static std::shared_ptr<Object> resolve(Graphics& gfx, const std::string& tag);
   static std::string generateUID(const std::string& tag);
   std::string getUID() const noexcept override;

   void draw() noexcept override;
   void freeUpload() noexcept override {};

   void createRootSignature();
   void createShader(const std::wstring& vertexPath, const std::wstring& pixelPath);
   void createPipeLineState(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology);

   void loadVerticsBuffer(const std::vector<Shape::VertexTexture> vertices);
   void loadIndicesBuffer(const std::vector<unsigned short> indices);
   //void createConstant(const XMFLOAT3 &colorB)
private:
   //enum class RootParameterType
   //{
   //   MatrixIndex,
   //   TextIndex,
   //   RootParameterCount
   //};

   Graphics& m_gfx;
   ID3D12Device* m_device;
   ID3D12GraphicsCommandList* m_commandList;
   ComPtr<ID3D12RootSignature> m_rootSignature;
   ComPtr<ID3DBlob>m_vertexShaderBlob;
   ComPtr<ID3DBlob>m_pixelShaderBlob;
   ComPtr<ID3D12PipelineState> m_pipelineState;
   ComPtr <ID3D12Resource> m_vertexDefaultBuffer;
   ComPtr <ID3D12Resource> m_vertexUploadBuffer;
   D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
   ComPtr <ID3D12Resource> m_indexDefaultBuffer;
   ComPtr <ID3D12Resource> m_indexUploadBuffer;
   D3D12_INDEX_BUFFER_VIEW m_indexBufferView;


   D3D12_PRIMITIVE_TOPOLOGY_TYPE m_topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
   std::string tag;

};

