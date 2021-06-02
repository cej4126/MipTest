#include "TextureMipmap.h"
#include "Surface.h"
#include "d3dx12.h"
#include "dds.h"

TextureMipmap::TextureMipmap(Graphics &gfx, std::string tag)
   :
   m_gfx(gfx),
   m_device(gfx.getDevice()),
   m_commandList(gfx.getCommandList())
{
}

std::shared_ptr<TextureMipmap> TextureMipmap::resolve(Graphics &gfx, const std::string &tag)
{
   return Bind::BindableCodex::resolve<TextureMipmap>(gfx, tag);
}

std::string TextureMipmap::generateUID(const std::string &tag)
{
   return typeid(TextureMipmap).name() + std::string("#") + tag;
}

std::string TextureMipmap::getUID() const noexcept
{
   return generateUID(tag);
}

void TextureMipmap::Bind(Graphics &gfx) noexcept
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

void TextureMipmap::createTextureMipmap(std::string path, int slot, int rootPara)
{
   FileDDS fileDDS;

   fileDDS.readDDSFile(UTF8ToWideString(path));
   m_rootPara = rootPara;
   //   m_alphaGloss = surface.AlphaLoaded();

   D3D12_HEAP_PROPERTIES heapProps;
   ZeroMemory(&heapProps, sizeof(heapProps));
   heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
   heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
   heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
   heapProps.CreationNodeMask = 1;
   heapProps.VisibleNodeMask = 1;

   D3D12_RESOURCE_DESC resourceDesc;
   ZeroMemory(&resourceDesc, sizeof(resourceDesc));
   resourceDesc.Alignment = 0;
   resourceDesc.Width = fileDDS.getWidth();
   resourceDesc.Height = fileDDS.getHeight();
   resourceDesc.DepthOrArraySize = fileDDS.getArraySize();
   resourceDesc.MipLevels = fileDDS.getMipCount();
   resourceDesc.Format = fileDDS.getFormat();
   resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
   resourceDesc.SampleDesc.Count = 1;
   resourceDesc.SampleDesc.Quality = 0;
   resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
   resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
   // only for 2d testing
   assert(fileDDS.getResourceDim() == D3D12_RESOURCE_DIMENSION_TEXTURE2D);
   resourceDesc.Dimension = fileDDS.getResourceDim();

   ThrowIfFailed(m_device->CreateCommittedResource(
      &heapProps,
      D3D12_HEAP_FLAG_NONE,
      &resourceDesc,
      D3D12_RESOURCE_STATE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&m_textureBuffers[slot])));

   // Upload heap
   heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
   UINT64 textureUploadBufferSize;
   m_device->GetCopyableFootprints(&resourceDesc, 0, fileDDS.getMipCount(), 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);
   //need 256
   const size_t AlignmentMask = DEFAULT_ALIGN - 1;
   const size_t AlignedSize = AlignUpWithMask(textureUploadBufferSize, AlignmentMask);

   resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
   resourceDesc.Alignment = 0;
   resourceDesc.DepthOrArraySize = 1;
   resourceDesc.MipLevels = 1;
   resourceDesc.SampleDesc.Count = 1;
   resourceDesc.SampleDesc.Quality = 0;
   resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
   resourceDesc.Width = AlignedSize;
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

   //// copy data to the upload heap
   //D3D12_SUBRESOURCE_DATA TextureData = {};
   //TextureData.pData = fileDDS.getSubData();
   //TextureData.RowPitch = fileDDS.getWidth() * sizeof(Surface::Color);
   //TextureData.SlicePitch = fileDDS.getWidth() * fileDDS.getHeight();
   int numberOfResource = fileDDS.getMipCount();
   D3D12_SUBRESOURCE_DATA *pSrcData = fileDDS.getSubData();

   UpdateSubresources(m_commandList, m_textureBuffers[slot].Get(), m_textureBufferUploadHeaps[slot].Get(), 0, 0, numberOfResource, pSrcData);


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
   // No multi array for testing
   assert(fileDDS.getArraySize() == 1);
   srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
   srvDesc.Texture2D.MipLevels = fileDDS.getMipCount();
   srvDesc.Texture2D.MostDetailedMip = 0;

   D3D12_CPU_DESCRIPTOR_HANDLE handle = m_mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
   int size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   handle.ptr += (SIZE_T)(slot * size);

   m_device->CreateShaderResourceView(m_textureBuffers[slot].Get(), &srvDesc, handle);

}
