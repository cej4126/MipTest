#pragma once
#include "stdafx.h"
#include "Window.h"

class App
{
public:
   App();
   int Go();
   ~App();
private:
   void DoFrame();
   float TimeMark();

   Window m_window;

   // Time
   std::chrono::steady_clock::time_point m_lastTime;
   float m_speedFactor = 1.0f;
};

