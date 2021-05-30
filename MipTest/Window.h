#pragma once
#include "stdafx.h"
#include "Input.h"
#include "Graphics.h"

class Window
{
public:
   Window(int width, int height);
   Window(const Window &) = delete;
   Window &operator=(const Window &) = delete;
   static std::optional<int> ProcessMessages() noexcept;
   ~Window();

   Graphics &gfx();

   void enableCursor() noexcept;
   void disableCursor() noexcept;
   bool isCursorEnabled() const noexcept { return m_cursorEnabled; }

   bool isRunning() { return m_running; }
   Input m_input;

private:
   static LRESULT CALLBACK HandleMessageInit(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
   static LRESULT CALLBACK HandleMessageMain(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
   LRESULT  HandleMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
   void confineCursor() noexcept;
   void freeCursor() noexcept;
   void showCursor() noexcept;
   void hideCursor() noexcept;
   void enableImGuiMouse() noexcept;
   void disableImGuiMouse() noexcept;
   int m_width;
   int m_height;
   HWND m_hWnd;
   HINSTANCE m_hInstance;
   const wchar_t *m_WindowName = L"MipTest";
   bool m_running = true;
   bool m_cursorEnabled = true;
   std::vector<BYTE> m_rawBuffer;
   std::unique_ptr<Graphics>pGfx;
   bool m_graphicActive = false;
};

