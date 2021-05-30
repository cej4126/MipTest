#pragma once
#include "stdafx.h"
#include "Bindable.h"
#include "Graphics.h"
#include "FileDDS.h"

class TextureMipmap : public Bind::Bindable
{
public:
   TextureMipmap(Graphics &gfx, std::string tag);

   static std::shared_ptr<TextureMipmap> resolve(Graphics &gfx, const std::string &tag);
   static std::string generateUID(const std::string &tag);
   std::string getUID() const noexcept override;

   void Bind(Graphics &gfx) noexcept override;

   void createTextureMipmap(std::string path, int slot, int rootPara);
   bool getAlphaGloss() { return m_alphaGloss; }

private:

   Graphics &m_gfx;
   ID3D12Device *m_device;
   ID3D12GraphicsCommandList *m_commandList;
   //ComPtr<ID3D12RootSignature> m_rootSignature;

   //DXGI_FORMAT getDXGIFormat(const DDS_PIXELFORMAT &ddpf);
   //std::wstring UTF8ToWideString(const std::string &str);
   //bool readDDSFile(const std::wstring fileName);
   //HRESULT createTextureDss(const uint8_t *data, size_t size);
   //ByteArray readFile(const std::wstring &fileName);

   static const int NUMBER_OF_VIEW = 3;

   std::string tag;
   int m_rootPara = -1;
   bool m_alphaGloss = false;
   ComPtr < ID3D12Resource > m_textureBuffers[NUMBER_OF_VIEW];
   ComPtr < ID3D12DescriptorHeap > m_mainDescriptorHeap;
   ComPtr < ID3D12Resource > m_textureBufferUploadHeaps[NUMBER_OF_VIEW];
};

