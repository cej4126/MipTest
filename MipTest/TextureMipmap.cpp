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

void TextureMipmap::draw() noexcept
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


void TextureMipmap::createTextureNew(const wchar_t *path, int slot, int rootPara)
{
   DirectX::ScratchImage *imageData = new DirectX::ScratchImage();
   ThrowIfFailed(DirectX::LoadFromDDSFile(path, DirectX::DDS_FLAGS_NONE, nullptr, *imageData));

   const DirectX::TexMetadata &textureMetaData = imageData->GetMetadata();
   DXGI_FORMAT textureFormat = textureMetaData.format;
   bool is3DTexture = textureMetaData.dimension == DirectX::TEX_DIMENSION_TEXTURE3D;

   D3D12_RESOURCE_DESC textureDesc{};
   textureDesc.Format = textureFormat;
   textureDesc.Width = (uint32_t)textureMetaData.width;
   textureDesc.Height = (uint32_t)textureMetaData.height;
   textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
   textureDesc.DepthOrArraySize = is3DTexture ? (uint16_t)textureMetaData.depth : (uint16_t)textureMetaData.arraySize;
   textureDesc.MipLevels = (uint16_t)textureMetaData.mipLevels;
   textureDesc.SampleDesc.Count = 1;
   textureDesc.SampleDesc.Quality = 0;
   textureDesc.Dimension = is3DTexture ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
   textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
   textureDesc.Alignment = 0;

   D3D12_HEAP_PROPERTIES defaultProperties;
   defaultProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
   defaultProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
   defaultProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
   defaultProperties.CreationNodeMask = 0;
   defaultProperties.VisibleNodeMask = 0;

   ID3D12Resource *newTextureResource = NULL;
   ThrowIfFailed(m_device->CreateCommittedResource(&defaultProperties,
      D3D12_HEAP_FLAG_NONE,
      &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&newTextureResource)));

   D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
   if (is3DTexture)
   {
      assert(textureMetaData.arraySize == 6);

      shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
      shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      shaderResourceViewDesc.TextureCube.MostDetailedMip = 0;
      shaderResourceViewDesc.TextureCube.MipLevels = (uint32_t)textureMetaData.mipLevels;
      shaderResourceViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
   }

   // Upload heap
   //UINT numRows[MAX_TEXTURE_SUBRESOURCE_COUNT];
   //uint64_t rowSizesInBytes[MAX_TEXTURE_SUBRESOURCE_COUNT];
   //D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[MAX_TEXTURE_SUBRESOURCE_COUNT];

   D3D12_HEAP_PROPERTIES uploadProps;
   uploadProps.Type = D3D12_HEAP_TYPE_UPLOAD;
   uploadProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
   uploadProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
   uploadProps.CreationNodeMask = 1;
   uploadProps.VisibleNodeMask = 1;

   //         m_textureBufferUploadHeaps[slot].Get(),
//         0, 0, numberOfResource, pSrcData);
   UINT64 RequiredSize = 0;
   auto MemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) +
      sizeof(UINT) + sizeof(UINT64)) * textureMetaData.mipLevels;
   if (MemToAlloc > SIZE_MAX)
   {
      throw;
   }
   void *pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
   if (pMem == nullptr)
   {
      throw;
   }
   auto pLayouts = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT *>(pMem);
   auto pRowSizesInBytes = reinterpret_cast<UINT64 *>(pLayouts + textureMetaData.mipLevels);
   auto pNumRows = reinterpret_cast<UINT *>(pRowSizesInBytes + textureMetaData.mipLevels);
   const uint64_t numSubResources = textureMetaData.mipLevels * textureMetaData.arraySize;
   uint64_t textureMemorySize = 0;

   m_device->GetCopyableFootprints(&textureDesc,
      0,
      (uint32_t)numSubResources,
      0,
      pLayouts, pNumRows, pRowSizesInBytes, &textureMemorySize);


   //need 256
   const size_t AlignmentMask = DEFAULT_ALIGN - 1;
   const size_t AlignedSize = AlignUpWithMask(textureMemorySize, AlignmentMask);

   D3D12_RESOURCE_DESC uploadDesc{};

   uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
   uploadDesc.Alignment = 0;
   uploadDesc.DepthOrArraySize = 1;
   uploadDesc.MipLevels = 1;
   uploadDesc.SampleDesc.Count = 1;
   uploadDesc.SampleDesc.Quality = 0;
   uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
   uploadDesc.Width = AlignedSize;
   uploadDesc.Height = 1;
   uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
   uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

   ThrowIfFailed(m_device->CreateCommittedResource(
      &uploadProps,
      D3D12_HEAP_FLAG_NONE,
      &uploadDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&m_textureBufferUploadHeaps[slot])));

   int numberOfResource = textureMetaData.mipLevels;
   auto IntermediateDesc = m_textureBufferUploadHeaps[slot]->GetDesc();


   if (slot == 0)
   {
      // create the descriptor heap that will store our srv
      D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
      heapDesc.NumDescriptors = NUMBER_OF_VIEW;
      heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
      heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
      ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_mainDescriptorHeap)));
   }

   D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = m_mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
   int size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   srcHandle.ptr += (SIZE_T)slot * (SIZE_T)size;

   m_device->CreateShaderResourceView(newTextureResource,
      is3DTexture ? &shaderResourceViewDesc : NULL,
      srcHandle);

   // UpdateSubresources 1(m_commandList, m_textureBuffers[slot].Get(),

   //D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts[11];
   //UINT NumRows[11];
   //UINT64 RowSizesInBytes[11];
   //for (int i = 0; i < 11; i++)
   //{
   //   Layouts[i] = pLayouts[i];
   //   NumRows[i] = pNumRows[i];
   //   RowSizesInBytes[i] = pRowSizesInBytes[i];
   //}

   //pDevice->GetCopyableFootprints(&Desc,
   //        FirstSubresource,
   //        NumSubresources,
   //        IntermediateOffset,
   //        pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);

   // UpdateSubresources 2


   BYTE *pData;
   HRESULT hr = m_textureBufferUploadHeaps[slot]->Map(0, nullptr, reinterpret_cast<void **>(&pData));
   if (FAILED(hr))
   {
      throw;
   }


   //const size_t AlignmentMask = DEFAULT_ALIGN - 1;
   //Direct3DUploadInfo uploadInfo = uploadContext->BeginUpload(textureMemorySize, mQueueManager);
   //uint8_t *uploadMemory = reinterpret_cast<uint8 *>(uploadInfo.Memory);

   for (uint64_t arrayIndex = 0; arrayIndex < textureMetaData.arraySize; arrayIndex++)
   {
      for (uint64_t mipIndex = 0; mipIndex < textureMetaData.mipLevels; mipIndex++)
      {
         const uint64_t subResourceIndex = mipIndex + (arrayIndex * textureMetaData.mipLevels);

         const D3D12_PLACED_SUBRESOURCE_FOOTPRINT &subResourceLayout = pLayouts[subResourceIndex];
         const uint64_t subResourceHeight = pNumRows[subResourceIndex];

         const uint64_t subResourcePitch = AlignUpWithMask(subResourceLayout.Footprint.RowPitch, AlignmentMask);
         const uint64_t subResourceDepth = subResourceLayout.Footprint.Depth;
         uint8_t *destinationSubResourceMemory = pData + subResourceLayout.Offset;

         for (uint64_t sliceIndex = 0; sliceIndex < subResourceDepth; sliceIndex++)
         { 
            const DirectX::Image *subImage = imageData->GetImage(mipIndex, arrayIndex, sliceIndex);
            const uint8_t *sourceSubResourceMemory = subImage->pixels;

            for (uint64_t height = 0; height < subResourceHeight; height++)
            {
               size_t size;
               if (subResourcePitch > subImage->rowPitch)
               {
                  size = subImage->rowPitch;
               }
               else
               {
                  size = subResourcePitch;
               }
               memcpy(destinationSubResourceMemory, sourceSubResourceMemory, size);
               destinationSubResourceMemory += subResourcePitch;
               sourceSubResourceMemory += subImage->rowPitch;
            }
         }
      }
   }


   HeapFree(GetProcessHeap(), 0, pMem);


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
   resourceDesc.Height = (UINT)fileDDS.getHeight();
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
   heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
   heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
   heapProps.CreationNodeMask = 1;
   heapProps.VisibleNodeMask = 1;

   //need 256
   UINT64 textureUploadBufferSize;

   m_device->GetCopyableFootprints(&resourceDesc, 0, fileDDS.getMipCount(), 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);
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

   //// copy data to the upload heap
   //D3D12_SUBRESOURCE_DATA TextureData = {};
   //TextureData.pData = fileDDS.getSubData();
   //TextureData.RowPitch = fileDDS.getWidth() * sizeof(Surface::Color);
   //TextureData.SlicePitch = fileDDS.getWidth() * fileDDS.getHeight();
   int numberOfResource = fileDDS.getMipCount() * fileDDS.getArraySize();
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
   srvDesc.Format = fileDDS.getFormat();
   // No multi array for testing
   assert(fileDDS.getArraySize() == 1);
   srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
   srvDesc.Texture2D.MipLevels = fileDDS.getMipCount();
   srvDesc.Texture2D.MostDetailedMip = 0;

   D3D12_CPU_DESCRIPTOR_HANDLE handle = m_mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
   int size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   handle.ptr += (SIZE_T)slot * (SIZE_T)size;

   m_device->CreateShaderResourceView(m_textureBuffers[slot].Get(), &srvDesc, handle);

}
