#pragma once
#include "stdafx.h"
#include "DirectXTex.h"
#include "dds.h"

#define ISBITMASK( r,g,b,a ) ( ddpf.RBitMask == r && ddpf.GBitMask == g && ddpf.BBitMask == b && ddpf.ABitMask == a )

static DXGI_FORMAT GetDXGIFormat(const DDS_PIXELFORMAT &ddpf);

static void GetSurfaceInfo(_In_ size_t width,
   _In_ size_t height,
   _In_ DXGI_FORMAT fmt,
   _Out_opt_ size_t *outNumBytes,
   _Out_opt_ size_t *outRowBytes,
   _Out_opt_ size_t *outNumRows);

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
   _Out_writes_(mipCount *arraySize) D3D12_SUBRESOURCE_DATA *initData);


//std::wstring UTF8ToWideString(const std::string &str);

class FileDDS
{
public:
   bool readDDSFile(const std::wstring fileName);
   typedef std::shared_ptr<std::vector<byte> > ByteArray;
   UINT64 getWidth() { return m_width; }
   UINT64 getHeight() { return m_height; }
   UINT64 getDepth() { return m_depth; }
   UINT16 getMipCount() { return m_mipCount; }
   DXGI_FORMAT getFormat() { return m_format; }
   UINT16 getArraySize() { return m_arraySize; }
   D3D12_RESOURCE_DIMENSION getResourceDim() { return m_resourceDim; }
   D3D12_SUBRESOURCE_DATA *getSubData() { return m_subData.get(); }

private:
   UINT64 m_width = 0;
   UINT64 m_height = 0;
   UINT64 m_depth = 1;
   UINT16 m_mipCount = 0;
   DXGI_FORMAT m_format;
   UINT16 m_arraySize;
   D3D12_RESOURCE_DIMENSION m_resourceDim = D3D12_RESOURCE_DIMENSION_UNKNOWN;
   std::unique_ptr<D3D12_SUBRESOURCE_DATA[]> m_subData;
   ByteArray byteArray;

   HRESULT createTextureDss(const uint8_t *data, size_t size);
   ByteArray readFile(const std::wstring &fileName);
};