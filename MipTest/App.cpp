#include "App.h"

App::App()
   :
   m_window(1000, 752)
{
   m_lastTime = std::chrono::steady_clock::now();

}

int App::Go()
{
   while (m_window.isRunning())
   {
      if (const auto code = Window::ProcessMessages())
      {
         return *code;
      }

      if (m_window.isRunning())
      {
         DoFrame();
      }
   }
   return 0;
}

App::~App()
{
}

void App::DoFrame()
{
   auto deltaTime = TimeMark() * m_speedFactor;

}

float App::TimeMark()
{
   const auto lastTime = m_lastTime;
   m_lastTime = std::chrono::steady_clock::now();
   const std::chrono::duration<float> frameTime = m_lastTime - lastTime;
   return frameTime.count();
}
