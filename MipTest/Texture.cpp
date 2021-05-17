#include "Texture.h"
#include "Surface.h"

Texture::Texture(Graphics &gfx, std::string tag)
   :
   m_gfx(gfx),
   m_device(gfx.getDevice()),
   m_commandList(gfx.getCommandList())
{
}

std::shared_ptr<Texture> Texture::resolve(Graphics &gfx, const std::string &tag)
{
   return Bind::BindableCodex::resolve<Texture>(gfx, tag);
}

std::string Texture::generateUID(const std::string &tag)
{
   return typeid(Texture).name() + std::string("#") + tag;
}

std::string Texture::getUID() const noexcept
{
   return generateUID(tag);
}

void Texture::Bind(Graphics &gfx) noexcept
{
   if (m_rootPara != -1)
   {
      // set the descriptor heap
      ID3D12DescriptorHeap *descriptorHeaps[] = { m_mainDescriptorHeap.Get() };
      m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

      // set the descriptor table to the descriptor heap (parameter 1, as constant buffer root descriptor is parameter index 0)
      m_commandList->SetGraphicsRootDescriptorTable(m_rootPara, m_mainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
   }
}


void Texture::createTexture(std::string path, int slot, int rootPara)
{
   const auto surface = Surface::FromFile(path);
   m_rootPara = rootPara;
   m_alphaGloss = surface.AlphaLoaded();

   D3D12_HEAP_PROPERTIES heapProps;
   ZeroMemory(&heapProps, sizeof(heapProps));
   heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
   heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
   heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
   heapProps.CreationNodeMask = 1;
   heapProps.VisibleNodeMask = 1;

   D3D12_RESOURCE_DESC resourceDesc;
   ZeroMemory(&resourceDesc, sizeof(resourceDesc));
   resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
   resourceDesc.Alignment = 0;
   resourceDesc.Width = surface.GetWidth();
   resourceDesc.Height = surface.GetHeight();
   resourceDesc.DepthOrArraySize = 1;
   resourceDesc.MipLevels = 1;
   resourceDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
   resourceDesc.SampleDesc.Count = 1;
   resourceDesc.SampleDesc.Quality = 0;
   resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
   resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

   ThrowIfFailed(m_device->CreateCommittedResource(
      &heapProps,
      D3D12_HEAP_FLAG_NONE,
      &resourceDesc,
      D3D12_RESOURCE_STATE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&m_textureBuffers[slot])));
   m_textureBuffers[slot]->SetName(L"Texture Default Buffer");

   // Upload heap
   heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
   UINT64 textureUploadBufferSize;
   m_device->GetCopyableFootprints(&resourceDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

   resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
   resourceDesc.Width = textureUploadBufferSize;
   resourceDesc.Height = 1;
   resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
   resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

   ThrowIfFailed(m_device->CreateCommittedResource(
      &heapProps,
      D3D12_HEAP_FLAG_NONE,
      &resourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&m_textureBufferUploadHeaps[slot])));
   m_textureBufferUploadHeaps[slot]->SetName(L"Texture Upload Buffer");

   // copy data to the upload heap
   D3D12_SUBRESOURCE_DATA TextureData = {};
   TextureData.pData = surface.GetBufferPtr();
   TextureData.RowPitch = surface.GetWidth() * sizeof(Surface::Color);
   TextureData.SlicePitch = surface.GetWidth() * sizeof(Surface::Color) * surface.GetHeight();

   m_gfx.UpdateSubresource(
      m_textureBuffers[slot].Get(),
      m_textureBufferUploadHeaps[slot].Get(),
      &TextureData); // pSrcData

   D3D12_RESOURCE_BARRIER resourceBarrier;
   resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
   resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
   resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
   resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
   resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
   resourceBarrier.Transition.pResource = m_textureBuffers[slot].Get();
   m_commandList->ResourceBarrier(1, &resourceBarrier);

   if (slot == 0)
   {
      // create the descriptor heap that will store our srv
      D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
      heapDesc.NumDescriptors = NUMBER_OF_VIEW;
      heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
      heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
      ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_mainDescriptorHeap)));
   }

   // now we create a shader resource view (descriptor that points to the texture and describes it)
   D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
   srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
   srvDesc.Format = resourceDesc.Format;
   srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
   srvDesc.Texture2D.MipLevels = 1;

   D3D12_CPU_DESCRIPTOR_HANDLE handle = m_mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
   int size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   handle.ptr += slot * size;

   m_device->CreateShaderResourceView(m_textureBuffers[slot].Get(), &srvDesc, handle);
}
