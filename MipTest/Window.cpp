#include "Window.h"

Window::Window(int width, int height)
   :
   m_width(width),
   m_height(height)
{
   m_hInstance = GetModuleHandle(nullptr);

   WNDCLASSEX windowClass = { 0 };
   windowClass.cbSize = sizeof(windowClass);
   windowClass.cbClsExtra = 0;
   windowClass.cbWndExtra = 0;
   windowClass.hbrBackground = nullptr;
   windowClass.hCursor = nullptr;
   windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
   windowClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
   windowClass.hInstance = m_hInstance;
   windowClass.lpfnWndProc = HandleMessageInit;
   windowClass.lpszClassName = m_WindowName;
   windowClass.style = CS_OWNDC;
   windowClass.lpszMenuName = nullptr;
   RegisterClassEx(&windowClass);

   RECT windowRect;
   windowRect.left = 120;
   windowRect.right = width + windowRect.left;
   windowRect.top = 100;
   windowRect.bottom = height + windowRect.top;
   if (AdjustWindowRect(&windowRect, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, FALSE) == 0)
   {
      throw;
   }

   m_hWnd = CreateWindow(
      m_WindowName, m_WindowName,
      WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
      CW_USEDEFAULT, CW_USEDEFAULT, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
      nullptr, nullptr, m_hInstance, this);
   if (m_hWnd == nullptr)
   {
      throw;
   }

   ShowWindow(m_hWnd, SW_SHOWDEFAULT);

}

Window::~Window()
{
   DestroyWindow(m_hWnd);
}

std::optional<int> Window::ProcessMessages() noexcept
{
   MSG message;

   while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
   {
      if (message.message == WM_QUIT)
      {
         return (int)message.wParam;
      }

      TranslateMessage(&message);
      DispatchMessage(&message);
   }

   return {};
}

LRESULT CALLBACK Window::HandleMessageInit(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   if (msg == WM_CREATE)
   {
      const CREATESTRUCT *const pStruct = reinterpret_cast<CREATESTRUCT *>(lParam);
      Window *const pWindow = static_cast<Window *>(pStruct->lpCreateParams);
      SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
      SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::HandleMessageMain));
   }
   return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK Window::HandleMessageMain(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   Window *const pWindow = reinterpret_cast<Window *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
   return pWindow->HandleMsg(hwnd, msg, wParam, lParam);
}

LRESULT Window::HandleMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   {
      case WM_CLOSE:
         m_running = false;
         PostQuitMessage(0);
         return 0;
      default:
         break;
   }
   return DefWindowProc(hwnd, msg, wParam, lParam);
}
