#include "Surface.h"
#include <wchar.h>
#include <filesystem>

Surface::Surface(unsigned int width, unsigned int height)
{
   HRESULT hr = scratch.Initialize2D(format, width, height, 1u, 1u);
   if (FAILED(hr))
   {
      throw ("Failed to initialize ScratchImage");
   }
}

void Surface::Clear(Color fillValue) noexcept
{
   const auto width = GetWidth();
   const auto height = GetHeight();
   auto &imageData = *scratch.GetImage(0, 0, 0);
   for (size_t y = 0u; y < height; y++)
   {
      auto rowStart = reinterpret_cast<Color *>(imageData.pixels + imageData.rowPitch * y);
      std::fill(rowStart, rowStart + imageData.width, fillValue);
   }
}

void Surface::PutPixel(unsigned int x, unsigned int y, Color c) noexcept
{
   assert(x >= 0);
   assert(y >= 0);
   assert(x < GetWidth());
   assert(y < GetHeight());
   auto &imageData = *scratch.GetImage(0, 0, 0);
   reinterpret_cast<Color *>(&imageData.pixels[y * imageData.rowPitch])[x] = c;
}

Surface::Color Surface::GetPixel(unsigned int x, unsigned int y) const noexcept
{
   assert(x >= 0);
   assert(y >= 0);
   assert(x < GetWidth());
   assert(y < GetHeight());
   auto &imageData = *scratch.GetImage(0, 0, 0);
   return reinterpret_cast<Color *>(&imageData.pixels[y * imageData.rowPitch])[x];
}

unsigned int Surface::GetWidth() const noexcept
{
   return (unsigned int)scratch.GetMetadata().width;
}

unsigned int Surface::GetHeight() const noexcept
{
   return (unsigned int)scratch.GetMetadata().height;
}

Surface::Color *Surface::GetBufferPtr() noexcept
{
   return reinterpret_cast<Color *>(scratch.GetPixels());
}

const Surface::Color *Surface::GetBufferPtr() const noexcept
{
   return const_cast<Surface *>(this)->GetBufferPtr();
}

const Surface::Color *Surface::GetBufferPtrConst() const noexcept
{
   return const_cast<Surface *>(this)->GetBufferPtr();
}

Surface Surface::FromFile(const std::string &filename)
{
   wchar_t wideName[512];
   mbstowcs_s(nullptr, wideName, filename.c_str(), _TRUNCATE);

   DirectX::ScratchImage scratch;
   HRESULT hr = DirectX::LoadFromWICFile(wideName, DirectX::WIC_FLAGS_NONE, nullptr, scratch);
   if (FAILED(hr))
   {
      throw ("Failed to load image");
   }

   if (scratch.GetImage(0, 0, 0)->format != format)
   {
      DirectX::ScratchImage converted;
      hr = DirectX::Convert(
         *scratch.GetImage(0, 0, 0),
         format,
         DirectX::TEX_FILTER_DEFAULT,
         DirectX::TEX_THRESHOLD_DEFAULT,
         converted);
      if (FAILED(hr))
      {
         throw ("Failed to convert image");
      }
      return Surface(std::move(converted));
   }
   return Surface(std::move(scratch));
}

void Surface::Save(const std::string &filename) const
{
   const auto GetCodecID = [](const std::string &filename)
   {
      const std::filesystem::path path = filename;
      const auto ext = path.extension().string();

      if (ext == ".png")
      {
         return DirectX::WIC_CODEC_PNG;
      }
      else if (ext == ".jpg")
      {
         return DirectX::WIC_CODEC_JPEG;
      }
      else if (ext == ".bmp")
      {
         return DirectX::WIC_CODEC_BMP;
      }
      throw ("Image format not supported");
   };

   wchar_t wideName[512];
   mbstowcs_s(nullptr, wideName, filename.c_str(), _TRUNCATE);

   HRESULT hr = DirectX::SaveToWICFile(
      *scratch.GetImage(0, 0, 0),
      DirectX::WIC_FLAGS_NONE,
      GetWICCodec(GetCodecID(filename)),
      wideName);

   if (FAILED(hr))
   {
      throw ("Failed to save image");
   }
}

bool Surface::AlphaLoaded() const noexcept
{
   return !scratch.IsAlphaAllOpaque();
}

Surface::Surface(DirectX::ScratchImage scratch) noexcept
   :
   scratch(std::move(scratch))
{
}
