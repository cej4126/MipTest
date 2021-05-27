#pragma once
#include "stdafx.h"
#include "Window.h"
#include "DrawPictureCube.h"
#include "Camera.h"
#include "d2write.h"

class App
{
public:
   App();
   int Go();
   ~App();
private:
   void DoFrame();
   float TimeMark();

   void spawnSimulation();

   Window m_window;

   // Time
   std::chrono::steady_clock::time_point m_lastTime;
   float m_speedFactor = 1.0f;

   std::unique_ptr<d2write> m_d2writeItem;
   Camera m_camera;

   //std::unique_ptr<class DrawPictureCube> cube;
   std::vector<std::unique_ptr<class DrawFunction>> m_drawItems;
   int m_drawCount = 0;
};

