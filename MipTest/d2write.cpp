#include "d2write.h"

d2write::d2write(Graphics &gfx)
   :
   m_gfx(gfx)
{
   // DWrite
   ThrowIfFailed(gfx.get2dContext()->CreateSolidColorBrush(
      D2D1::ColorF(D2D1::ColorF::White),
      &m_x11d2dtextBrush));
   ThrowIfFailed(gfx.get2dWriteFactory()->CreateTextFormat(
      L"Arial",
      NULL,
      DWRITE_FONT_WEIGHT_NORMAL,
      DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL,
      25,
      L"en-us",
      &m_x11d2dtextFormat
   ));
}

void d2write::draw()
{
   float textX1 = 200;
   float textX2 = 950;
   float textY = 150;

   static const WCHAR textTitle[] = L"MIPMAP test";
   D2D1_RECT_F titleRect = D2D1::RectF(20.0f, 20.0f, 200.0f, 50.0f);

   static const WCHAR textTexture[] = L"Texture";
   D2D1_RECT_F textureRect = D2D1::RectF(textX1, textY, textX1+200.0f, textY+50.0f);

   static const WCHAR textMipMap[] = L"MIPMAP";
   D2D1_RECT_F mipmapRect = D2D1::RectF(textX2, textY, textX2+200.0f, textY+50.0f);

   m_gfx.get2dContext()->BeginDraw();

   //m_gfx.get2dContext()->DrawRectangle(D2D1::RectF(5.0f, 5.0f, 200.0f, 50.0f), m_x11d2dtextBrush.Get());
   //m_gfx.get2dContext()->SetTransform(D2D1::Matrix3x2F::Identity());
   m_gfx.get2dContext()->DrawText(textTitle, _countof(textTitle) - 1, m_x11d2dtextFormat.Get(), &titleRect, m_x11d2dtextBrush.Get());
   m_gfx.get2dContext()->DrawText(textTexture, _countof(textTexture) - 1, m_x11d2dtextFormat.Get(), &textureRect, m_x11d2dtextBrush.Get());
   m_gfx.get2dContext()->DrawText(textMipMap, _countof(textMipMap) - 1, m_x11d2dtextFormat.Get(), &mipmapRect, m_x11d2dtextBrush.Get());

   ThrowIfFailed(m_gfx.get2dContext()->EndDraw());

}
