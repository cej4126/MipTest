#include "Window.h"
#include "imgui/imgui.h"

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

   pGfx = std::make_unique<Graphics>(m_hWnd, m_width, m_height);
   m_graphicActive = true;

   ShowWindow(m_hWnd, SW_SHOWDEFAULT);

   RAWINPUTDEVICE rawInputDevice;
   rawInputDevice.dwFlags = 0;
   rawInputDevice.hwndTarget = nullptr;
   rawInputDevice.usUsage = 0x2;
   rawInputDevice.usUsagePage = 0x1;
   if (RegisterRawInputDevices(&rawInputDevice, 1, sizeof(rawInputDevice)) == FALSE)
   {
      throw;
   }
}

Window::~Window()
{
   DestroyWindow(m_hWnd);
}

Graphics &Window::gfx()
{
   if (!pGfx)
   {
      throw;
   }
   return *pGfx;
}

void Window::enableCursor() noexcept
{
   m_cursorEnabled = true;
   showCursor();
   enableImGuiMouse();
   freeCursor();
}

void Window::disableCursor() noexcept
{
   m_cursorEnabled = false;
   hideCursor();
   disableImGuiMouse();
   confineCursor();
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
         pGfx->cleanUp();

         PostQuitMessage(0);
         return 0;
      case WM_KILLFOCUS:
         m_input.clear();
         break;
      case WM_ACTIVATE:
         if (!m_cursorEnabled)
         {
            if (wParam & WA_ACTIVE)
            {
               confineCursor();
               hideCursor();
            }
            else
            {
               freeCursor();
               showCursor();
            }
         }
         break;
      case WM_KEYDOWN:
      case WM_SYSKEYDOWN:
         if (!(lParam & 0x40000000) || (m_input.isAutoRepeatEnable()))
         {
            m_input.onKeyBoardPressed(static_cast<unsigned char>(wParam));
         }
         break;
      case WM_KEYUP:
      case WM_SYSKEYUP:
         m_input.onKeyBoardRelease(static_cast<unsigned char>(wParam));
         break;
      case WM_CHAR:
         m_input.onChar(static_cast<unsigned char>(wParam));
         break;
      case WM_MOUSEMOVE:
         if (m_cursorEnabled)
         {
            const POINTS points = MAKEPOINTS(lParam);
            if ((points.x >= 0) && (points.x < m_width) && (points.y >= 0) && (points.y < m_height))
            {
               m_input.onMouseMove(points.x, points.y);
               if (!m_input.isInWindow())
               {
                  SetCapture(m_hWnd);
                  m_input.onMouseEnter();
               }
            }
            else
            {
               if (wParam & (MK_LBUTTON | MK_RBUTTON))
               {
                  m_input.onMouseMove(points.x, points.y);
               }
               else
               {
                  ReleaseCapture();
                  m_input.onMouseLeave();
               }
            }
         }
         else
         {
            if (!m_input.isInWindow())
            {
               SetCapture(m_hWnd);
               m_input.onMouseEnter();
               hideCursor();
            }
         }

         break;

      case WM_LBUTTONDOWN:
      {
         SetForegroundWindow(m_hWnd);
         if (!m_cursorEnabled)
         {
            confineCursor();
            hideCursor();
         }

         m_input.onLeftPressed();
         break;
      }
      case WM_RBUTTONDOWN:
      {
         m_input.onRightPressed();
         break;
      }
      case WM_LBUTTONUP:
      {
         const POINTS points = MAKEPOINTS(lParam);
         m_input.onLeftReleased();
         // release mouse if outside of window
         if (points.x < 0 || points.x >= m_width || points.y < 0 || points.y >= m_height)
         {
            ReleaseCapture();
            m_input.onMouseLeave();
         }
         break;
      }
      case WM_RBUTTONUP:
      {
         const POINTS points = MAKEPOINTS(lParam);
         m_input.onRightReleased();
         // release mouse if outside of window
         if (points.x < 0 || points.x >= m_width || points.y < 0 || points.y >= m_height)
         {
            ReleaseCapture();
            m_input.onMouseLeave();
         }
         break;
      }
      case WM_MOUSEWHEEL:
      {
         const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
         m_input.onWheelDelta(delta);
         break;
      }

      case WM_INPUT:
      {
         if (!m_input.isRawEnabled())
         {
            break;
         }

         UINT size;
         // first get the size of the input data
         if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) == -1)
         {
            // bail msg processing if error
            break;
         }
         m_rawBuffer.resize(size);
         // read in the input data
         if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, m_rawBuffer.data(), &size, sizeof(RAWINPUTHEADER)) != size)
         {
            // bail msg processing if error
            break;
         }
         // process the raw input data
         auto &rawInput = reinterpret_cast<const RAWINPUT &>(*m_rawBuffer.data());
         if (rawInput.header.dwType == RIM_TYPEMOUSE &&
            (rawInput.data.mouse.lLastX != 0 || rawInput.data.mouse.lLastY != 0))
         {
            m_input.onRawDelta(rawInput.data.mouse.lLastX, rawInput.data.mouse.lLastY);
         }
         break;
      }


      default:
         break;
   }
   return DefWindowProc(hwnd, msg, wParam, lParam);
}

void Window::confineCursor() noexcept
{
   RECT rect;
   GetClientRect(m_hWnd, &rect);
   MapWindowPoints(m_hWnd, nullptr, reinterpret_cast<POINT *>(&rect), 2);
   ClipCursor(&rect);
}

void Window::freeCursor() noexcept
{
   ClipCursor(nullptr);
}

void Window::showCursor() noexcept
{
   while (::ShowCursor(TRUE) < 0);
}

void Window::hideCursor() noexcept
{
   while (::ShowCursor(FALSE) >= 0);
}

void Window::enableImGuiMouse() noexcept
{
   ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
}

void Window::disableImGuiMouse() noexcept
{
   ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
}
