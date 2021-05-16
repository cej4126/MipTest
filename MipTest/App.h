#pragma once
#include "stdafx.h"
#include "Window.h"
#include "DrawPictureCube.h"
#include "Camera.h"

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

   Camera m_camera;

   std::unique_ptr<class DrawPictureCube> cube;
   int m_drawCount = 0;
};

