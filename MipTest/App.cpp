#include "App.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

App::App()
   :
   m_window(1280, 720)
{
   m_lastTime = std::chrono::steady_clock::now();

   m_d2writeItem = std::make_unique<d2write>(m_window.gfx());

   m_drawItems.push_back(std::make_unique<DrawPictureCube>(m_window.gfx(), m_drawCount, "..\\..\\MipTest\\Images\\280893.jpg"));
   m_drawItems.at(size_t((size_t)m_drawCount - 1))->setPos(XMFLOAT3(0.0f, 1.1f, 1.1f));

   m_drawItems.push_back(std::make_unique<DrawPictureCubeTex>(m_window.gfx(), m_drawCount, "..\\..\\MipTest\\Images\\280893.dds"));
   m_drawItems.at(size_t((size_t)m_drawCount - 1))->setPos(XMFLOAT3(0.0f, 1.1f, -1.1f));

   //m_drawItems.push_back(std::make_unique<DrawPictureCube>(m_window.gfx(), m_drawCount, "..\\..\\MipTest\\Images\\cobalt-city.jpg"));
   //m_drawItems.at(size_t((size_t)m_drawCount - 1))->setPos(XMFLOAT3(0.0f, -1.1f, 1.1f));

   m_drawItems.push_back(std::make_unique<DrawPictureCubeTex>(m_window.gfx(), m_drawCount, "..\\..\\MipTest\\Images\\280893 - Copy.dds"));
   m_drawItems.at(size_t((size_t)m_drawCount - 1))->setPos(XMFLOAT3(0.0f, -1.1f, -1.1f));

   //m_drawItems.push_back(std::make_unique<DrawPictureCubeTex>(m_window.gfx(), m_drawCount, "..\\..\\MipTest\\Images\\cobalt-city.dds"));
   //m_drawItems.at(size_t((size_t)m_drawCount - 1))->setPos(XMFLOAT3(0.0f, -1.1f, -1.1f));


   m_window.gfx().createMatrixConstant(m_drawCount);

   m_window.gfx().runCommandList();
   m_window.gfx().setProjection(XMMatrixPerspectiveLH(1.0f, 9.0f / 16.0f, 0.5f, 400.0f));
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

   m_window.gfx().setCamera(m_camera.getMatrix());
   m_window.gfx().onRenderBegin();
   m_window.gfx().onRender();

   for (auto &b : m_drawItems)
   {
      b->update(deltaTime);
      b->draw();
   }

   m_d2writeItem->draw();

   // Process input
   while (auto e = m_window.m_input.readKey())
   {
      if (e->isPress())
      {
         switch (e->getCode())
         {
            case VK_ESCAPE:
               if (m_window.isCursorEnabled())
               {
                  m_window.disableCursor();
                  m_window.m_input.enableRaw();
               }
               else
               {
                  m_window.enableCursor();
                  m_window.m_input.disableRaw();
               }
               break;
         }
      }
   }

   if (m_window.m_input.isKeyPressed('W'))
   {
      m_camera.translate({ 0.0f, 0.0f, deltaTime });
   }
   else if (m_window.m_input.isKeyPressed('S'))
   {
      m_camera.translate({ 0.0f, 0.0f, -deltaTime });
   }
   else if (m_window.m_input.isKeyPressed('A'))
   {
      m_camera.translate({ -deltaTime, 0.0f, 0.0f });
   }
   else if (m_window.m_input.isKeyPressed('D'))
   {
      m_camera.translate({ deltaTime, 0.0f, 0.0f });
   }
   else if (m_window.m_input.isKeyPressed('Q'))
   {
      m_camera.translate({ 0.0f, deltaTime, 0.0f });
   }
   else if (m_window.m_input.isKeyPressed('E'))
   {
      m_camera.translate({ 0.0f, -deltaTime, 0.0f });
   }

   while (const auto delta = m_window.m_input.readRawDelta())
   {
      if (!m_window.isCursorEnabled())
      {
         m_camera.rotate((float)delta->x, (float)delta->y);
      }
   }

   spawnSimulation();

   m_camera.createControlWindow();

   ImGui::Render();
   ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

   m_window.gfx().drawCommandList();

   m_window.gfx().onRenderEnd();
}

float App::TimeMark()
{
   const auto lastTime = m_lastTime;
   m_lastTime = std::chrono::steady_clock::now();
   const std::chrono::duration<float> frameTime = m_lastTime - lastTime;
   return frameTime.count();
}

void App::spawnSimulation()
{
   // Start the ImGui frame
   ImGui_ImplDX11_NewFrame();
   ImGui_ImplWin32_NewFrame();
   ImGui::NewFrame();
   ImGui::Begin("Simulation Speed");

   ImGui::SliderFloat("Speed Factor", &m_speedFactor, 0.0f, 4.0f);
   ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
      1000.0f / ImGui::GetIO().Framerate,
      ImGui::GetIO().Framerate);
   ImGui::End();
}
