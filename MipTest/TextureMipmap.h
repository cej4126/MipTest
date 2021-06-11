#pragma once
#include "stdafx.h"
#include "Bindable.h"
#include "Graphics.h"
#include "FileDDS.h"

class TextureMipmap : public Bind::Bindable
{
public:
   TextureMipmap(Graphics& gfx, std::string tag);

   static std::shared_ptr<TextureMipmap> resolve(Graphics& gfx, const std::string& tag);
   static std::string generateUID(const std::string& tag);
   std::string getUID() const noexcept override;

   void draw() noexcept override;
   void freeUpload() noexcept override;

   void createTextureMipmap(const wchar_t* path, int slot, int rootPara);
   //void createTextureMipmap(std::string path, int slot, int rootPara);
   bool getAlphaGloss() { return m_alphaGloss; }

private:
#define DEFAULT_ALIGN 256
   template <typename T> __forceinline T AlignUpWithMask(T value, size_t mask)
   {
      return (T)(((size_t)value + mask) & ~mask);
   }

   Graphics& m_gfx;
   ID3D12Device* m_device;
   ID3D12GraphicsCommandList* m_commandList;

   static const int NUMBER_OF_VIEW = 3;

   std::string tag;
   int m_rootPara = -1;
   bool m_alphaGloss = false;
   ComPtr < ID3D12Resource > m_textureBuffers[NUMBER_OF_VIEW];
   ComPtr < ID3D12DescriptorHeap > m_mainDescriptorHeap;
   ComPtr < ID3D12Resource > m_textureBufferUploadHeaps[NUMBER_OF_VIEW];
};

