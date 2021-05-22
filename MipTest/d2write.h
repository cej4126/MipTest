#pragma once
#include "stdafx.h"
#include "Graphics.h"

class d2write
{
public:
   d2write(Graphics &gfx);
   void draw();

private:
   Graphics &m_gfx;

   Microsoft::WRL::ComPtr<IDWriteTextFormat> m_x11d2dtextFormat;
   Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_x11d2dtextBrush;
};

