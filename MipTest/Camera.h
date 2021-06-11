#pragma once
#include "stdafx.h"
#include "Graphics.h"

class Camera
{
public:
   Camera() noexcept;
   XMMATRIX getMatrix() const noexcept;
   void createControlWindow() noexcept;
   void reset() noexcept;
   void rotate(float dx, float dy) noexcept;
   void translate(XMFLOAT3 translation) noexcept;

private:
   float wrapAngle(float theta);
   static constexpr float PI = 3.14159265f;
   static constexpr float TravelSpeed = 12.0;
   static constexpr float RotationSpeed = 0.001f;
   XMFLOAT3 m_position;
   float m_pitch;
   float m_yaw;

};

