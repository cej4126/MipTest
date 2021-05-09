#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <Winuser.h>
#include <optional>
#include <memory>
#include <vector>
#include <array>
#include <wrl.h>
#include <chrono>
#include <random>
#include <optional>

#include <string>
#include <assert.h>
#include <memory>
#include <unordered_map>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// DirectX 11
#include <d3d11on12.h>

// DWrite
#include <d2d1_3.h>
#include <dwrite.h>

inline void ThrowIfFailed(HRESULT hr)
{
   if (hr != S_OK)
   {
      throw;
   }
}