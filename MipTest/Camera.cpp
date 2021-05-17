#include "Camera.h"
#include "imgui/imgui.h"

Camera::Camera() noexcept
{
   reset();
}

XMMATRIX Camera::getMatrix() const noexcept
{
   const XMVECTOR forwardBaseVector = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
   const auto lookVector = XMVector3Transform(forwardBaseVector,
      XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f)
   );

   const auto camPosition = XMLoadFloat3(&m_position);
   const auto camTarget = camPosition + lookVector;
   return XMMatrixLookAtLH(camPosition, camTarget, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
}

void Camera::createControlWindow() noexcept
{
   if (ImGui::Begin("Camera Control"))
   {
      ImGui::Text("Position");
      ImGui::SliderFloat("X", &m_position.x, -80.0f, 80.0f, "%.1f");
      ImGui::SliderFloat("Y", &m_position.y, -80.0f, 80.0f, "%.1f");
      ImGui::SliderFloat("Z", &m_position.z, -80.0f, 80.0f, "%.1f");
      ImGui::Text("Orientation");
      ImGui::SliderAngle("Pitch", &m_pitch, -89.950f, 89.95f);
      ImGui::SliderAngle("Yaw", &m_yaw, -180.0f, 180.0f);
      if (ImGui::Button("Reset"))
      {
         reset();
      }
   }
   ImGui::End();
}

void Camera::reset() noexcept
{
   m_position = { -7.5f, 0.0f, 0.0f };
   m_pitch = 0.0f;
   m_yaw = PI / 2.0f;
}

void Camera::rotate(float dx, float dy) noexcept
{
   m_yaw = wrapAngle(m_yaw + dx * RotationSpeed);
   m_pitch = std::clamp(m_pitch + dy * RotationSpeed, -PI / 2.0f, PI / 2.0f);
}

void Camera::translate(XMFLOAT3 translation) noexcept
{
   XMStoreFloat3(&translation, XMVector3Transform(
      XMLoadFloat3(&translation),
      XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f) *
      XMMatrixScaling(TravelSpeed, TravelSpeed, TravelSpeed)
   ));
   m_position = {
      m_position.x + translation.x,
      m_position.y + translation.y,
      m_position.z + translation.z
   };
}

float Camera::wrapAngle(float theta)
{
   constexpr float twoPi = 2.0f * PI;
   const float mod = fmod(theta, twoPi);
   if (mod > PI)
   {
      return mod - twoPi;
   }
   else if (mod < PI)
   {
      return mod + twoPi;
   }
   return mod;
}
