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


std::wstring UTF8ToWideString(const std::string &str);

class FileDDS
{
public:
   bool readDDSFile(const std::wstring fileName);
   typedef std::shared_ptr<std::vector<byte> > ByteArray;

private:
   HRESULT createTextureDss(const uint8_t *data, size_t size);
   ByteArray readFile(const std::wstring &fileName);
};