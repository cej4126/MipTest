#include "FileDDS.h"

static DXGI_FORMAT GetDXGIFormat(const DDS_PIXELFORMAT &ddpf)
{
   if (ddpf.flags & DDS_RGB)
   {
      // Note that sRGB formats are written using the "DX10" extended header

      switch (ddpf.RGBBitCount)
      {
         case 32:
            if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
            {
               return DXGI_FORMAT_R8G8B8A8_UNORM;
            }

            if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
            {
               return DXGI_FORMAT_B8G8R8A8_UNORM;
            }

            if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000))
            {
               return DXGI_FORMAT_B8G8R8X8_UNORM;
            }

            // No DXGI format maps to ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0x00000000) aka D3DFMT_X8B8G8R8

            // Note that many common DDS reader/writers (including D3DX) swap the
            // the RED/BLUE masks for 10:10:10:2 formats. We assumme
            // below that the 'backwards' header mask is being used since it is most
            // likely written by D3DX. The more robust solution is to use the 'DX10'
            // header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

            // For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
            if (ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
            {
               return DXGI_FORMAT_R10G10B10A2_UNORM;
            }

            // No DXGI format maps to ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) aka D3DFMT_A2R10G10B10

            if (ISBITMASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
            {
               return DXGI_FORMAT_R16G16_UNORM;
            }

            if (ISBITMASK(0xffffffff, 0x00000000, 0x00000000, 0x00000000))
            {
               // Only 32-bit color channel format in D3D9 was R32F
               return DXGI_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114
            }
            break;

         case 24:
            // No 24bpp DXGI formats aka D3DFMT_R8G8B8
            break;

         case 16:
            if (ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x8000))
            {
               return DXGI_FORMAT_B5G5R5A1_UNORM;
            }
            if (ISBITMASK(0xf800, 0x07e0, 0x001f, 0x0000))
            {
               return DXGI_FORMAT_B5G6R5_UNORM;
            }

            // No DXGI format maps to ISBITMASK(0x7c00,0x03e0,0x001f,0x0000) aka D3DFMT_X1R5G5B5

            if (ISBITMASK(0x0f00, 0x00f0, 0x000f, 0xf000))
            {
               return DXGI_FORMAT_B4G4R4A4_UNORM;
            }

            // No DXGI format maps to ISBITMASK(0x0f00,0x00f0,0x000f,0x0000) aka D3DFMT_X4R4G4B4

            // No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8, D3DFMT_A8P8, etc.
            break;
      }
   }
   else if (ddpf.flags & DDS_LUMINANCE)
   {
      if (8 == ddpf.RGBBitCount)
      {
         if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x00000000))
         {
            return DXGI_FORMAT_R8_UNORM; // D3DX10/11 writes this out as DX10 extension
         }

         // No DXGI format maps to ISBITMASK(0x0f,0x00,0x00,0xf0) aka D3DFMT_A4L4
      }

      if (16 == ddpf.RGBBitCount)
      {
         if (ISBITMASK(0x0000ffff, 0x00000000, 0x00000000, 0x00000000))
         {
            return DXGI_FORMAT_R16_UNORM; // D3DX10/11 writes this out as DX10 extension
         }
         if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
         {
            return DXGI_FORMAT_R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension
         }
      }
   }
   else if (ddpf.flags & DDS_ALPHA)
   {
      if (8 == ddpf.RGBBitCount)
      {
         return DXGI_FORMAT_A8_UNORM;
      }
   }
   else if (ddpf.flags & DDS_FOURCC)
   {
      if (MAKEFOURCC('D', 'X', 'T', '1') == ddpf.fourCC)
      {
         return DXGI_FORMAT_BC1_UNORM;
      }
      if (MAKEFOURCC('D', 'X', 'T', '3') == ddpf.fourCC)
      {
         return DXGI_FORMAT_BC2_UNORM;
      }
      if (MAKEFOURCC('D', 'X', 'T', '5') == ddpf.fourCC)
      {
         return DXGI_FORMAT_BC3_UNORM;
      }

      // While pre-mulitplied alpha isn't directly supported by the DXGI formats,
      // they are basically the same as these BC formats so they can be mapped
      if (MAKEFOURCC('D', 'X', 'T', '2') == ddpf.fourCC)
      {
         return DXGI_FORMAT_BC2_UNORM;
      }
      if (MAKEFOURCC('D', 'X', 'T', '4') == ddpf.fourCC)
      {
         return DXGI_FORMAT_BC3_UNORM;
      }

      if (MAKEFOURCC('A', 'T', 'I', '1') == ddpf.fourCC)
      {
         return DXGI_FORMAT_BC4_UNORM;
      }
      if (MAKEFOURCC('B', 'C', '4', 'U') == ddpf.fourCC)
      {
         return DXGI_FORMAT_BC4_UNORM;
      }
      if (MAKEFOURCC('B', 'C', '4', 'S') == ddpf.fourCC)
      {
         return DXGI_FORMAT_BC4_SNORM;
      }

      if (MAKEFOURCC('A', 'T', 'I', '2') == ddpf.fourCC)
      {
         return DXGI_FORMAT_BC5_UNORM;
      }
      if (MAKEFOURCC('B', 'C', '5', 'U') == ddpf.fourCC)
      {
         return DXGI_FORMAT_BC5_UNORM;
      }
      if (MAKEFOURCC('B', 'C', '5', 'S') == ddpf.fourCC)
      {
         return DXGI_FORMAT_BC5_SNORM;
      }

      // BC6H and BC7 are written using the "DX10" extended header

      if (MAKEFOURCC('R', 'G', 'B', 'G') == ddpf.fourCC)
      {
         return DXGI_FORMAT_R8G8_B8G8_UNORM;
      }
      if (MAKEFOURCC('G', 'R', 'G', 'B') == ddpf.fourCC)
      {
         return DXGI_FORMAT_G8R8_G8B8_UNORM;
      }

      if (MAKEFOURCC('Y', 'U', 'Y', '2') == ddpf.fourCC)
      {
         return DXGI_FORMAT_YUY2;
      }

      // Check for D3DFORMAT enums being set here
      switch (ddpf.fourCC)
      {
         case 36: // D3DFMT_A16B16G16R16
            return DXGI_FORMAT_R16G16B16A16_UNORM;

         case 110: // D3DFMT_Q16W16V16U16
            return DXGI_FORMAT_R16G16B16A16_SNORM;

         case 111: // D3DFMT_R16F
            return DXGI_FORMAT_R16_FLOAT;

         case 112: // D3DFMT_G16R16F
            return DXGI_FORMAT_R16G16_FLOAT;

         case 113: // D3DFMT_A16B16G16R16F
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

         case 114: // D3DFMT_R32F
            return DXGI_FORMAT_R32_FLOAT;

         case 115: // D3DFMT_G32R32F
            return DXGI_FORMAT_R32G32_FLOAT;

         case 116: // D3DFMT_A32B32G32R32F
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
      }
   }

   return DXGI_FORMAT_UNKNOWN;
}

//--------------------------------------------------------------------------------------
// Get surface information for a particular format
//--------------------------------------------------------------------------------------
static void GetSurfaceInfo(_In_ size_t width,
   _In_ size_t height,
   _In_ DXGI_FORMAT fmt,
   _Out_opt_ size_t *outNumBytes,
   _Out_opt_ size_t *outRowBytes,
   _Out_opt_ size_t *outNumRows)
{
   size_t numBytes = 0;
   size_t rowBytes = 0;
   size_t numRows = 0;

   bool bc = false;
   bool packed = false;
   bool planar = false;
   size_t bpe = 0;
   switch (fmt)
   {
      case DXGI_FORMAT_BC1_TYPELESS:
      case DXGI_FORMAT_BC1_UNORM:
      case DXGI_FORMAT_BC1_UNORM_SRGB:
      case DXGI_FORMAT_BC4_TYPELESS:
      case DXGI_FORMAT_BC4_UNORM:
      case DXGI_FORMAT_BC4_SNORM:
         bc = true;
         bpe = 8;
         break;

      case DXGI_FORMAT_BC2_TYPELESS:
      case DXGI_FORMAT_BC2_UNORM:
      case DXGI_FORMAT_BC2_UNORM_SRGB:
      case DXGI_FORMAT_BC3_TYPELESS:
      case DXGI_FORMAT_BC3_UNORM:
      case DXGI_FORMAT_BC3_UNORM_SRGB:
      case DXGI_FORMAT_BC5_TYPELESS:
      case DXGI_FORMAT_BC5_UNORM:
      case DXGI_FORMAT_BC5_SNORM:
      case DXGI_FORMAT_BC6H_TYPELESS:
      case DXGI_FORMAT_BC6H_UF16:
      case DXGI_FORMAT_BC6H_SF16:
      case DXGI_FORMAT_BC7_TYPELESS:
      case DXGI_FORMAT_BC7_UNORM:
      case DXGI_FORMAT_BC7_UNORM_SRGB:
         bc = true;
         bpe = 16;
         break;

      case DXGI_FORMAT_R8G8_B8G8_UNORM:
      case DXGI_FORMAT_G8R8_G8B8_UNORM:
      case DXGI_FORMAT_YUY2:
         packed = true;
         bpe = 4;
         break;

      case DXGI_FORMAT_Y210:
      case DXGI_FORMAT_Y216:
         packed = true;
         bpe = 8;
         break;

      case DXGI_FORMAT_NV12:
      case DXGI_FORMAT_420_OPAQUE:
         planar = true;
         bpe = 2;
         break;

      case DXGI_FORMAT_P010:
      case DXGI_FORMAT_P016:
         planar = true;
         bpe = 4;
         break;

   }

   if (bc)
   {
      size_t numBlocksWide = 0;
      if (width > 0)
      {
         numBlocksWide = std::max<size_t>(1, (width + 3) / 4);
      }
      size_t numBlocksHigh = 0;
      if (height > 0)
      {
         numBlocksHigh = std::max<size_t>(1, (height + 3) / 4);
      }
      rowBytes = numBlocksWide * bpe;
      numRows = numBlocksHigh;
      numBytes = rowBytes * numBlocksHigh;
   }
   else if (packed)
   {
      rowBytes = ((width + 1) >> 1) * bpe;
      numRows = height;
      numBytes = rowBytes * height;
   }
   else if (fmt == DXGI_FORMAT_NV11)
   {
      rowBytes = ((width + 3) >> 2) * 4;
      numRows = height * 2; // Direct3D makes this simplifying assumption, although it is larger than the 4:1:1 data
      numBytes = rowBytes * numRows;
   }
   else if (planar)
   {
      rowBytes = ((width + 1) >> 1) * bpe;
      numBytes = (rowBytes * height) + ((rowBytes * height + 1) >> 1);
      numRows = height + ((height + 1) >> 1);
   }
   else
   {
      size_t bpp = BitsPerPixel(fmt);
      rowBytes = (width * bpp + 7) / 8; // round up to nearest byte
      numRows = height;
      numBytes = rowBytes * height;
   }

   if (outNumBytes)
   {
      *outNumBytes = numBytes;
   }
   if (outRowBytes)
   {
      *outRowBytes = rowBytes;
   }
   if (outNumRows)
   {
      *outNumRows = numRows;
   }
}

//--------------------------------------------------------------------------------------
static HRESULT FillInitData(_In_ size_t width,
   _In_ size_t height,
   _In_ size_t depth,
   _In_ size_t mipCount,
   _In_ size_t arraySize,
   _In_ DXGI_FORMAT format,
   _In_ size_t maxsize,
   _In_ size_t bitSize,
   _In_reads_bytes_(bitSize) const uint8_t *bitData,
   _Out_ size_t &twidth,
   _Out_ size_t &theight,
   _Out_ size_t &tdepth,
   _Out_ size_t &skipMip,
   _Out_writes_(mipCount *arraySize) D3D12_SUBRESOURCE_DATA *initData)
{
   if (!bitData || !initData)
   {
      return E_POINTER;
   }

   skipMip = 0;
   twidth = 0;
   theight = 0;
   tdepth = 0;

   size_t NumBytes = 0;
   size_t RowBytes = 0;
   const uint8_t *pSrcBits = bitData;
   const uint8_t *pEndBits = bitData + bitSize;

   size_t index = 0;
   for (size_t j = 0; j < arraySize; j++)
   {
      size_t w = width;
      size_t h = height;
      size_t d = depth;
      for (size_t i = 0; i < mipCount; i++)
      {
         GetSurfaceInfo(w, h, format, &NumBytes, &RowBytes, nullptr);

         if ((mipCount <= 1) || !maxsize || (w <= maxsize && h <= maxsize && d <= maxsize))
         {
            if (!twidth)
            {
               twidth = w;
               theight = h;
               tdepth = d;
            }

            assert(index < mipCount *arraySize);
            _Analysis_assume_(index < mipCount *arraySize);
            initData[index].pData = (const void *)pSrcBits;
            initData[index].RowPitch = static_cast<UINT>(RowBytes);
            initData[index].SlicePitch = static_cast<UINT>(NumBytes);
            ++index;
         }
         else if (!j)
         {
            // Count number of skipped mipmaps (first item only)
            ++skipMip;
         }

         if (pSrcBits + (NumBytes * d) > pEndBits)
         {
            return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
         }

         pSrcBits += NumBytes * d;

         w = w >> 1;
         h = h >> 1;
         d = d >> 1;
         if (w == 0)
         {
            w = 1;
         }
         if (h == 0)
         {
            h = 1;
         }
         if (d == 0)
         {
            d = 1;
         }
      }
   }

   return (index > 0) ? S_OK : E_FAIL;
}

FileDDS::ByteArray NullFile = std::make_shared<std::vector<byte> >(std::vector<byte>());

HRESULT FileDDS::createTextureDss(const uint8_t *data, size_t size)
{
   if ((size == 0) || (!data))
   {
      return E_INVALIDARG;
   }

   if (size < (sizeof(uint32_t) + sizeof(DDS_HEADER)))
   {
      return E_FAIL;
   }

   uint32_t dwMagicNumber = *(const uint32_t *)(data);
   if (dwMagicNumber != DDS_MAGIC)
   {
      return E_FAIL;
   }

   auto header = reinterpret_cast<const DDS_HEADER *>(data + sizeof(uint32_t));
   if ((header->size != sizeof(DDS_HEADER) ||
      (header->ddspf.size != sizeof(DDS_PIXELFORMAT))))
   {
      return E_FAIL;
   }

   size_t offset = sizeof(DDS_HEADER) + sizeof(uint32_t);

   if (header->ddspf.flags & DDS_FOURCC)
   {
      if (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC)
      {
         offset += sizeof(DDS_HEADER_DXT10);
      }
   }

   if (size < offset)
   {
      return E_FAIL;
   }

   UINT width = header->width;
   UINT height = header->height;
   UINT depth = header->depth;

   uint32_t resDim = D3D12_RESOURCE_DIMENSION_UNKNOWN;
   UINT arraySize = 1;
   DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
   bool isCubeMap = false;

   size_t mipCount = header->mipMapCount;
   if (mipCount == 0)
   {
      mipCount = 1;
   }

   if ((header->ddspf.flags & DDS_FOURCC) && (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC))
   {
      auto d3d10ext = reinterpret_cast<const DDS_HEADER_DXT10 *>((const char *)header + sizeof(DDS_HEADER));

      arraySize = d3d10ext->arraySize;
      if (arraySize == 0)
      {
         return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
      }

      format = d3d10ext->dxgiFormat;

      switch (format)
      {
         case DXGI_FORMAT_AI44:
         case DXGI_FORMAT_IA44:
         case DXGI_FORMAT_P8:
         case DXGI_FORMAT_A8P8:
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
         default:
            if (BitsPerPixel(format) == 0)
            {
               return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            }
      }

      switch (d3d10ext->resourceDimension)
      {
         case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            if ((header->flags == DDS_HEIGHT) && (height != 1))
            {
               return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }
            height = 1;
            depth = 1;
            break;
         case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if (d3d10ext->miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE)
            {
               arraySize *= 6;
               isCubeMap = true;
            }
            depth = 1;
            break;
         case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            if (!(header->flags & DDS_HEADER_FLAGS_VOLUME))
            {
               return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (arraySize > 1)
            {
               return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            }
            break;
         default:
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
      }

      resDim = d3d10ext->resourceDimension;
   }
   else
   {
      // !((header->ddspf.flags & DDS_FOURCC) && (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC))
      format = GetDXGIFormat(header->ddspf);

      if (format == DXGI_FORMAT_UNKNOWN)
      {
         return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
      }

      if (header->flags & DDS_HEADER_FLAGS_VOLUME)
      {
         resDim = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
      }
      else
      {
         if (header->caps2 & DDS_CUBEMAP)
         {
            // We require all six faces to be defined
            if ((header->caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
            {
               return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            }

            arraySize = 6;
            isCubeMap = true;
         }

         depth = 1;
         resDim = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

         // Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
      }

      assert(BitsPerPixel(format) != 0);
   }

   // Bound sizes (for security purposes we don't trust DDS file metadata larger than the D3D 11.x hardware requirements)
   if (mipCount > D3D12_REQ_MIP_LEVELS)
   {
      return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
   }

   switch (resDim)
   {
      case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
         if ((arraySize > D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION) ||
            (width > D3D12_REQ_TEXTURE1D_U_DIMENSION))
         {
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
         }
         break;

      case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
         if (isCubeMap)
         {
            // This is the right bound because we set arraySize to (NumCubes*6) above
            if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
               (width > D3D12_REQ_TEXTURECUBE_DIMENSION) ||
               (height > D3D12_REQ_TEXTURECUBE_DIMENSION))
            {
               return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            }
         }
         else if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
            (width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) ||
            (height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION))
         {
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
         }
         break;

      case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
         if ((arraySize > 1) ||
            (width > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
            (height > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
            (depth > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION))
         {
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
         }
         break;

      default:
         return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
   }

   // Create the texture
   UINT subresourceCount = static_cast<UINT>(mipCount) * arraySize;
   //std::unique_ptr<D3D12_SUBRESOURCE_DATA[]> initData(new (std::nothrow) D3D12_SUBRESOURCE_DATA[subresourceCount]);
   m_subData = std::make_unique<D3D12_SUBRESOURCE_DATA[]>(subresourceCount);

   if (!m_subData)
   {
      return E_OUTOFMEMORY;
   }

   size_t skipMip = 0;
   size_t twidth = 0;
   size_t theight = 0;
   size_t tdepth = 0;
   HRESULT hr = E_FAIL;

   //3dDevice,
   //   header,
   const uint8_t *bitData = data + offset;
   size_t bitSize = size - offset;
   size_t maxsize = 0;
   bool forceSRGB = false;
   //         texture,
   //         textureView );

   hr = FillInitData(width, height, depth, mipCount, arraySize, format, maxsize, bitSize, bitData,
      twidth, theight, tdepth, skipMip, m_subData.get());

   m_width = (UINT64)twidth;
   m_height = (UINT64)theight;
   m_depth = (UINT64)tdepth;
   m_mipCount = (UINT16)(mipCount - skipMip);
   m_format = format;
   m_arraySize = (UINT16)arraySize;
   m_resourceDim = (D3D12_RESOURCE_DIMENSION)resDim;
   //m_subData = initData.get();


   //int testdata[11][10];
   //LONG_PTR row[11];
   //LONG_PTR slice[11];
   //for (int i = 0; i < 11; i++)
   //{
   //   int *ptr = (int *)m_subData[i].pData;
   //   for (int j = 0; j < 10; j++)
   //   {
   //      testdata[i][j] = ptr[j];
   //   }
   //   row[i] = m_subData[i].RowPitch;
   //   slice[i] = m_subData[i].SlicePitch;
   //}

   return S_OK;
}

bool FileDDS::readDDSFile(const std::wstring fileName)
{
   ByteArray data = readFile(fileName);
   HRESULT hr = createTextureDss((const uint8_t *)data->data(), data->size());
   if (hr != S_OK)
   {
      return false;
   }

   return false;
}

FileDDS::ByteArray FileDDS::readFile(const std::wstring &fileName)
{
   struct _stat64 fileStat;
   int fileExists = _wstat64(fileName.c_str(), &fileStat);
   if (fileExists == -1)
   {
      return NullFile;
   }

   std::ifstream file(fileName, std::ios::in | std::ios::binary);
   if (!file)
   {
      return NullFile;
   }

   byteArray = std::make_shared<std::vector<byte> >(fileStat.st_size);
   file.read((char *)byteArray->data(), byteArray->size());
   file.close();

   return byteArray;
}
