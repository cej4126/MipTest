#pragma once
#include "stdafx.h"
#include "Input.h"

class Window
{
public:
   Window(int width, int height);
   Window(const Window &) = delete;
   Window &operator=(const Window &) = delete;
   static std::optional<int> ProcessMessages() noexcept;
   ~Window();

   bool isRunning() { return m_running; }
   Input input;

private:
   static LRESULT CALLBACK HandleMessageInit(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
   static LRESULT CALLBACK HandleMessageMain(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
   LRESULT  HandleMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
   void ConfineCursor() noexcept;
   void FreeCursor() noexcept;
   void ShowCursor() noexcept;
   void HideCursor() noexcept;

   int m_width;
   int m_height;
   HWND m_hWnd;
   HINSTANCE m_hInstance;
   static constexpr const char *m_WindowName = "MipTest";
   bool m_running = true;
   bool m_cursorEnabled = true;
   std::vector<BYTE> m_rawBuffer;
};

