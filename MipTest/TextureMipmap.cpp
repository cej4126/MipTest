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
   srvDesc.Texture2D.MipLevels = (!fileDDS.getMipCount()) ? -1 : resourceDesc.MipLevels;

   D3D12_CPU_DESCRIPTOR_HANDLE handle = m_mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
   int size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   handle.ptr += (SIZE_T)(slot * size);

   m_device->CreateShaderResourceView(m_textureBuffers[slot].Get(), &srvDesc, handle);

}
//
//TextureMipmap::ByteArray NullFile = std::make_shared<std::vector<byte> >(std::vector<byte>());
//
//TextureMipmap::ByteArray TextureMipmap::readFile(const std::wstring &fileName)
//{
//   struct _stat64 fileStat;
//   int fileExists = _wstat64(fileName.c_str(), &fileStat);
//   if (fileExists == -1)
//   {
//      return NullFile;
//   }
//
//   std::ifstream file(fileName, std::ios::in | std::ios::binary);
//   if (!file)
//   {
//      return NullFile;
//   }
//
//   ByteArray byteArray = std::make_shared<std::vector<byte> >(fileStat.st_size);
//   file.read((char *)byteArray->data(), byteArray->size());
//   file.close();
//
//   return byteArray;
//}
//
//HRESULT TextureMipmap::createTextureDss(const uint8_t *data, size_t size)
//{
//   if ((size == 0) || (!data))
//   {
//      return E_INVALIDARG;
//   }
//
//   if (size < (sizeof(uint32_t) + sizeof(DDS_HEADER)))
//   {
//      return E_FAIL;
//   }
//
//   uint32_t dwMagicNumber = *(const uint32_t *)(data);
//   if (dwMagicNumber != DDS_MAGIC)
//   {
//      return E_FAIL;
//   }
//
//   auto header = reinterpret_cast<const DDS_HEADER *>(data + sizeof(uint32_t));
//   if ((header->size != sizeof(DDS_HEADER) ||
//      (header->ddspf.size != sizeof(DDS_PIXELFORMAT))))
//   {
//      return E_FAIL;
//   }
//
//   size_t offset = sizeof(DDS_HEADER) + sizeof(uint32_t);
//
//   if (header->ddspf.flags & DDS_FOURCC)
//   {
//      if (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC)
//      {
//         offset += sizeof(DDS_HEADER_DXT10);
//      }
//   }
//
//   if (size < offset)
//   {
//      return E_FAIL;
//   }
//
//   UINT width = header->width;
//   UINT height = header->height;
//   UINT depth = header->depth;
//
//   uint32_t resDim = D3D12_RESOURCE_DIMENSION_UNKNOWN;
//   UINT arraySize = 1;
//   DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
//   bool isCubeMap = false;
//
//   size_t mipCount = header->mipMapCount;
//   if (mipCount == 0)
//   {
//      mipCount = 1;
//   }
//
//   if ((header->ddspf.flags & DDS_FOURCC) && (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC))
//   {
//      auto d3d10ext = reinterpret_cast<const DDS_HEADER_DXT10 *>((const char *)header + sizeof(DDS_HEADER));
//
//      arraySize = d3d10ext->arraySize;
//      if (arraySize == 0)
//      {
//         return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
//      }
//
//      format = d3d10ext->dxgiFormat;
//
//      switch (format)
//      {
//         case DXGI_FORMAT_AI44:
//         case DXGI_FORMAT_IA44:
//         case DXGI_FORMAT_P8:
//         case DXGI_FORMAT_A8P8:
//            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
//         default:
//            if (BitsPerPixel(format) == 0)
//            {
//               return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
//            }
//      }
//
//      switch (d3d10ext->resourceDimension)
//      {
//         case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
//            if ((header->flags == DDS_HEIGHT) && (height != 1))
//            {
//               return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
//            }
//            height = 1;
//            depth = 1;
//            break;
//         case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
//            if (d3d10ext->miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE)
//            {
//               arraySize *= 6;
//               isCubeMap = true;
//            }
//            depth = 1;
//            break;
//         case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
//            if (!(header->flags & DDS_HEADER_FLAGS_VOLUME))
//            {
//               return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
//            }
//
//            if (arraySize > 1)
//            {
//               return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
//            }
//            break;
//         default:
//            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
//      }
//
//      resDim = d3d10ext->resourceDimension;
//   }
//   else
//   {
//      // !((header->ddspf.flags & DDS_FOURCC) && (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC))
//      format = GetDXGIFormat(header->ddspf);
//
//      if (format == DXGI_FORMAT_UNKNOWN)
//      {
//         return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
//      }
//
//      if (header->flags & DDS_HEADER_FLAGS_VOLUME)
//      {
//         resDim = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
//      }
//      else
//      {
//         if (header->caps2 & DDS_CUBEMAP)
//         {
//            // We require all six faces to be defined
//            if ((header->caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
//            {
//               return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
//            }
//
//            arraySize = 6;
//            isCubeMap = true;
//         }
//
//         depth = 1;
//         resDim = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
//
//         // Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
//      }
//
//      assert(BitsPerPixel(format) != 0);
//   }
//
//   // Bound sizes (for security purposes we don't trust DDS file metadata larger than the D3D 11.x hardware requirements)
//   if (mipCount > D3D12_REQ_MIP_LEVELS)
//   {
//      return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
//   }
//
//   switch (resDim)
//   {
//      case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
//         if ((arraySize > D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION) ||
//            (width > D3D12_REQ_TEXTURE1D_U_DIMENSION))
//         {
//            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
//         }
//         break;
//
//      case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
//         if (isCubeMap)
//         {
//            // This is the right bound because we set arraySize to (NumCubes*6) above
//            if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
//               (width > D3D12_REQ_TEXTURECUBE_DIMENSION) ||
//               (height > D3D12_REQ_TEXTURECUBE_DIMENSION))
//            {
//               return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
//            }
//         }
//         else if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
//            (width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) ||
//            (height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION))
//         {
//            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
//         }
//         break;
//
//      case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
//         if ((arraySize > 1) ||
//            (width > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
//            (height > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
//            (depth > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION))
//         {
//            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
//         }
//         break;
//
//      default:
//         return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
//   }
//
//   {
//      // Create the texture
//      UINT subresourceCount = static_cast<UINT>(mipCount) * arraySize;
//      std::unique_ptr<D3D12_SUBRESOURCE_DATA[]> initData(new (std::nothrow) D3D12_SUBRESOURCE_DATA[subresourceCount]);
//      if (!initData)
//      {
//         return E_OUTOFMEMORY;
//      }
//
//      size_t skipMip = 0;
//      size_t twidth = 0;
//      size_t theight = 0;
//      size_t tdepth = 0;
//      hr = FillInitData(width, height, depth, mipCount, arraySize, format, maxsize, bitSize, bitData,
//         twidth, theight, tdepth, skipMip, initData.get());
//
//      if (SUCCEEDED(hr))
//      {
//         hr = CreateD3DResources(d3dDevice, resDim, twidth, theight, tdepth, mipCount - skipMip, arraySize,
//            format, forceSRGB,
//            isCubeMap, texture, textureView);
//
//         if (FAILED(hr) && !maxsize && (mipCount > 1))
//         {
//            // Retry with a maxsize determined by feature level
//            maxsize = (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
//               ? 2048 /*D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION*/
//               : 8192 /*D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION*/;
//
//            hr = FillInitData(width, height, depth, mipCount, arraySize, format, maxsize, bitSize, bitData,
//               twidth, theight, tdepth, skipMip, initData.get());
//            if (SUCCEEDED(hr))
//            {
//               hr = CreateD3DResources(d3dDevice, resDim, twidth, theight, tdepth, mipCount - skipMip, arraySize,
//                  format, forceSRGB,
//                  isCubeMap, texture, textureView);
//            }
//         }
//      }
//
//      if (SUCCEEDED(hr))
//      {
//         GpuResource DestTexture(*texture, D3D12_RESOURCE_STATE_COPY_DEST);
//         CommandContext::InitializeTexture(DestTexture, subresourceCount, initData.get());
//      }
//   }
//
//   return S_OK;
//}
//
//bool TextureMipmap::readDDSFile(const std::wstring fileName)
//{
//   ByteArray data = readFile(fileName);
//   HRESULT hr = createTextureDss((const uint8_t *)data->data(), data->size());
//   if (hr != S_OK)
//   {
//      return false;
//   }
//
//   return false;
//}
