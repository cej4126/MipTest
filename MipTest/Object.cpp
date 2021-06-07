#include "Object.h"
#include "d3dx12.h"

Object::Object(Graphics &gfx, std::string tag)
   :
   m_gfx(gfx),
   m_device(gfx.getDevice()),
   m_commandList(gfx.getCommandList())
{
}

std::shared_ptr<Object> Object::resolve(Graphics &gfx, const std::string &tag)
{
   return Bind::BindableCodex::resolve<Object>(gfx, tag);
}

std::string Object::generateUID(const std::string &tag)
{
   return typeid(Object).name() + std::string("#") + tag;
}

std::string Object::getUID() const noexcept
{
   return generateUID(tag);
}

void Object::draw() noexcept
{
   m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
   m_commandList->SetPipelineState(m_pipelineState.Get());

   m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
   m_commandList->IASetIndexBuffer(&m_indexBufferView);
}

void Object::createRootSignature()
{
   D3D12_ROOT_DESCRIPTOR rootDescriptor;
   rootDescriptor.RegisterSpace = 0;
   rootDescriptor.ShaderRegister = 0;

   D3D12_ROOT_PARAMETER rootParameters[2];

   // Root Paramter 0 -----------------
   // Constant buffer for matrix
   rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
   rootParameters[0].Descriptor = rootDescriptor;
   rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

   // Root Paramter 1 -----------------
   // Create descriptor range
   D3D12_DESCRIPTOR_RANGE tableRangeDescriptor[1];
   tableRangeDescriptor[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
   tableRangeDescriptor[0].NumDescriptors = 1;
   tableRangeDescriptor[0].BaseShaderRegister = 0;
   tableRangeDescriptor[0].RegisterSpace = 0;
   tableRangeDescriptor[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

   // Create descriptor table
   D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
   descriptorTable.NumDescriptorRanges = _countof(tableRangeDescriptor);
   descriptorTable.pDescriptorRanges = &tableRangeDescriptor[0];

   rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
   rootParameters[1].DescriptorTable = descriptorTable;
   rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

   // Sampler --------------------
   D3D12_STATIC_SAMPLER_DESC sampler = {};
   sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
   sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
   sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
   sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
   sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
   sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
   sampler.MaxAnisotropy = 0;
   sampler.MaxLOD = D3D12_FLOAT32_MAX;
   sampler.MinLOD = 0.0f;
   sampler.RegisterSpace = 0;
   sampler.ShaderRegister = 0;
   sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

   // Create Root Signature
   D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
   rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
   rootSignatureDesc.NumParameters = 2;
   rootSignatureDesc.pParameters = rootParameters;
   rootSignatureDesc.NumStaticSamplers = 1;
   rootSignatureDesc.pStaticSamplers = &sampler;

   ComPtr<ID3DBlob> signature;
   ComPtr<ID3DBlob> error;
   ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
   ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void Object::createShader(const std::wstring &vertexPath, const std::wstring &pixelPath)
{
   ThrowIfFailed(D3DReadFileToBlob(vertexPath.c_str(), m_vertexShaderBlob.ReleaseAndGetAddressOf()));
   ThrowIfFailed(D3DReadFileToBlob(pixelPath.c_str(), m_pixelShaderBlob.ReleaseAndGetAddressOf()));
}

void Object::createPipeLineState(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology)
{
   m_topology = topology;

   D3D12_RASTERIZER_DESC rasterizerDesc;
   ZeroMemory(&rasterizerDesc, sizeof(rasterizerDesc));
   rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
   rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
   rasterizerDesc.FrontCounterClockwise = FALSE;
   rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
   rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
   rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
   rasterizerDesc.DepthClipEnable = TRUE;
   rasterizerDesc.MultisampleEnable = FALSE;
   rasterizerDesc.AntialiasedLineEnable = FALSE;
   rasterizerDesc.ForcedSampleCount = 0;
   rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

   D3D12_BLEND_DESC blendDesc;
   ZeroMemory(&blendDesc, sizeof(blendDesc));
   blendDesc.AlphaToCoverageEnable = FALSE;
   blendDesc.IndependentBlendEnable = FALSE;
   blendDesc.RenderTarget[0] = {
      FALSE,FALSE,
      D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
      D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
      D3D12_LOGIC_OP_NOOP,
      D3D12_COLOR_WRITE_ENABLE_ALL
   };

   // Depth
   D3D12_DEPTH_STENCIL_DESC depthDesc = {};
   depthDesc.DepthEnable = TRUE;
   depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
   depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
   depthDesc.StencilEnable = FALSE;
   depthDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
   depthDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
   const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
   { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
   depthDesc.FrontFace = defaultStencilOp;
   depthDesc.BackFace = defaultStencilOp;

   // Define the vertex input layout.
   D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
   {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
   };

   // Describe and create the graphics pipeline state object (PSO).
   D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
   psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
   psoDesc.pRootSignature = m_rootSignature.Get();
   psoDesc.VS = { m_vertexShaderBlob->GetBufferPointer(), m_vertexShaderBlob->GetBufferSize() };
   psoDesc.PS = { m_pixelShaderBlob->GetBufferPointer(), m_pixelShaderBlob->GetBufferSize() };
   psoDesc.RasterizerState = rasterizerDesc;
   psoDesc.BlendState = blendDesc;
   psoDesc.SampleMask = UINT_MAX;
   psoDesc.PrimitiveTopologyType = m_topology;
   psoDesc.NumRenderTargets = 1;
   psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
   psoDesc.SampleDesc.Count = 1;
   psoDesc.DepthStencilState = depthDesc;
   psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
   ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void Object::loadVerticsBuffer(const std::vector<Shape::VertexTexture> vertices)
{
   const UINT vertexBufferSize = (UINT)(vertices.size() * sizeof(Shape::VertexTexture));

   D3D12_HEAP_PROPERTIES heapProps;
   ZeroMemory(&heapProps, sizeof(heapProps));
   heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
   heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
   heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
   heapProps.CreationNodeMask = 1;
   heapProps.VisibleNodeMask = 1;

   D3D12_RESOURCE_DESC resourceDesc;
   ZeroMemory(&resourceDesc, sizeof(resourceDesc));
   resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
   resourceDesc.Alignment = 0;
   resourceDesc.Width = vertexBufferSize;
   resourceDesc.Height = 1;
   resourceDesc.DepthOrArraySize = 1;
   resourceDesc.MipLevels = 1;
   resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
   resourceDesc.SampleDesc.Count = 1;
   resourceDesc.SampleDesc.Quality = 0;
   resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
   resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

   ThrowIfFailed(m_device->CreateCommittedResource(
      &heapProps,
      D3D12_HEAP_FLAG_NONE,
      &resourceDesc,
      D3D12_RESOURCE_STATE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&m_vertexDefaultBuffer)));

   // Upload heap
   heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

   ThrowIfFailed(m_device->CreateCommittedResource(
      &heapProps,
      D3D12_HEAP_FLAG_NONE,
      &resourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&m_vertexUploadBuffer)));

   // copy data to the upload heap
   D3D12_SUBRESOURCE_DATA vertexData = {};
   vertexData.pData = vertices.data(); //reinterpret_cast<BYTE *>(
   vertexData.RowPitch = vertexBufferSize;
   vertexData.SlicePitch = vertexBufferSize;

   // Add the copy to the command list
   //m_gfx.UpdateSubresource(
   //   m_vertexDefaultBuffer.Get(),
   //   m_vertexUploadBuffer.Get(),
   //   &vertexData); // pSrcData

   UpdateSubresources<1>(m_commandList, m_vertexDefaultBuffer.Get(), m_vertexUploadBuffer.Get(), 0, 0, 1, &vertexData);

   //UpdateSubresource(
   //   m_vertexDefaultBuffer.Get(),
   //   m_vertexUploadBuffer.Get(),
   //   &vertexData); // pSrcData

   D3D12_RESOURCE_BARRIER resourceBarrier;
   resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
   resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
   resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
   resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
   resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
   resourceBarrier.Transition.pResource = m_vertexDefaultBuffer.Get();
   m_commandList->ResourceBarrier(1, &resourceBarrier);

   // create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
   m_vertexBufferView.BufferLocation = m_vertexDefaultBuffer->GetGPUVirtualAddress();
   m_vertexBufferView.StrideInBytes = (UINT)sizeof(Shape::VertexTexture);
   m_vertexBufferView.SizeInBytes = vertexBufferSize;

}

void Object::loadIndicesBuffer(const std::vector<unsigned short> indices)
{
   const UINT indicesBufferSize = (UINT)(indices.size() * sizeof(unsigned short));

   D3D12_HEAP_PROPERTIES heapProps;
   ZeroMemory(&heapProps, sizeof(heapProps));
   heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
   heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
   heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
   heapProps.CreationNodeMask = 1;
   heapProps.VisibleNodeMask = 1;

   D3D12_RESOURCE_DESC resourceDesc;
   ZeroMemory(&resourceDesc, sizeof(resourceDesc));
   resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
   resourceDesc.Alignment = 0;
   resourceDesc.Width = indicesBufferSize;
   resourceDesc.Height = 1;
   resourceDesc.DepthOrArraySize = 1;
   resourceDesc.MipLevels = 1;
   resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
   resourceDesc.SampleDesc.Count = 1;
   resourceDesc.SampleDesc.Quality = 0;
   resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
   resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

   ThrowIfFailed(m_device->CreateCommittedResource(
      &heapProps,
      D3D12_HEAP_FLAG_NONE,
      &resourceDesc,
      D3D12_RESOURCE_STATE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&m_indexDefaultBuffer)));
   m_indexDefaultBuffer->SetName(L"index Default Buffer");

   // Upload heap
   heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

   ThrowIfFailed(m_device->CreateCommittedResource(
      &heapProps,
      D3D12_HEAP_FLAG_NONE,
      &resourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&m_indexUploadBuffer)));
   m_indexUploadBuffer->SetName(L"index Upload Buffer");

   // copy data to the upload heap
   D3D12_SUBRESOURCE_DATA indexData = {};
   indexData.pData = indices.data(); // reinterpret_cast<BYTE *>(indicesX12);
   indexData.RowPitch = indicesBufferSize;
   indexData.SlicePitch = indicesBufferSize;

   //m_gfx.UpdateSubresource(
   //   m_indexDefaultBuffer.Get(),
   //   m_indexUploadBuffer.Get(),
   //   &indexData); // pSrcData
   UpdateSubresources<1>(m_commandList, m_indexDefaultBuffer.Get(), m_indexUploadBuffer.Get(), 0, 0, 1, &indexData);

   D3D12_RESOURCE_BARRIER resourceBarrier;
   resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
   resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
   resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
   resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
   resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
   resourceBarrier.Transition.pResource = m_indexDefaultBuffer.Get();
   m_commandList->ResourceBarrier(1, &resourceBarrier);

   m_indexBufferView.BufferLocation = m_indexDefaultBuffer->GetGPUVirtualAddress();
   m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
   m_indexBufferView.SizeInBytes = indicesBufferSize;
}
