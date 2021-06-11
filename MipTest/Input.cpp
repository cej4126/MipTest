#include "Input.h"

std::optional<Input::RawDelta> Input::readRawDelta() noexcept
{
   if (m_rawDeltaBuffer.empty())
   {
      return std::nullopt;
   }
   const RawDelta delta = m_rawDeltaBuffer.front();
   m_rawDeltaBuffer.pop();
   return delta;
}

std::optional<Input::KeyBoardEvent> Input::readKey() noexcept
{
   if (m_keyBoardBuffer.size() > 0u)
   {
      Input::KeyBoardEvent keyEvent = m_keyBoardBuffer.front();
      m_keyBoardBuffer.pop();
      return keyEvent;
   }
   return {};
}

void Input::onKeyBoardPressed(unsigned char code) noexcept
{
   m_keyStates[code] = true;
   m_keyBoardBuffer.push(Input::KeyBoardEvent(Input::KeyBoardEvent::KeyBoardType::KeyBoardPress, code));
   trimKeyBoardBuffer();
}

void Input::onKeyBoardReleased(unsigned char code) noexcept
{
   m_keyStates[code] = false;
   m_keyBoardBuffer.push(Input::KeyBoardEvent(Input::KeyBoardEvent::KeyBoardType::KeyBoardRelease, code));
   trimKeyBoardBuffer();
}

void Input::onChar(char character) noexcept
{
   m_charBuffer.push(character);
   trimCharBuffer();
}

void Input::clear() noexcept
{
   m_keyStates.reset();
}

void Input::onMouseMove(int x, int y) noexcept
{
   m_x = x;
   m_y = y;
   m_mouseBuffer.push(Input::MouseEvent(Input::MouseEvent::MouseType::MouseMove, *this));
   trimMouseBuffer();
}

void Input::onMouseLeave() noexcept
{
   m_isInWindow = false;
   m_mouseBuffer.push(Input::MouseEvent(Input::MouseEvent::MouseType::LeaveWindow, *this));
   trimMouseBuffer();
}

void Input::onMouseEnter() noexcept
{
   m_isInWindow = true;
   m_mouseBuffer.push(Input::MouseEvent(Input::MouseEvent::MouseType::EnterWindow, *this));
   trimMouseBuffer();
}

void Input::onRawDelta(int dx, int dy) noexcept
{
   m_rawDeltaBuffer.push({ dx, dy });
   trimRawBuffer();
}

void Input::onLeftPressed() noexcept
{
   m_leftIsPressed = true;
   m_mouseBuffer.push(Input::MouseEvent(Input::MouseEvent::MouseType::LeftPress, *this));
   trimMouseBuffer();
}

void Input::onLeftReleased() noexcept
{
   m_leftIsPressed = false;
   m_mouseBuffer.push(Input::MouseEvent(Input::MouseEvent::MouseType::LeftRelease, *this));
   trimMouseBuffer();
}

void Input::onRightPressed() noexcept
{
   m_rightIsPressed = true;
   m_mouseBuffer.push(Input::MouseEvent(Input::MouseEvent::MouseType::RightPress, *this));
   trimMouseBuffer();
}

void Input::onRightReleased() noexcept
{
   m_rightIsPressed = false;
   m_mouseBuffer.push(Input::MouseEvent(Input::MouseEvent::MouseType::RightRelease, *this));
   trimMouseBuffer();
}

void Input::onWheelUp() noexcept
{
   m_mouseBuffer.push(Input::MouseEvent(Input::MouseEvent::MouseType::WheelUp, *this));
   trimMouseBuffer();
}

void Input::onWheelDown() noexcept
{
   m_mouseBuffer.push(Input::MouseEvent(Input::MouseEvent::MouseType::WheelDown, *this));
   trimMouseBuffer();
}

void Input::trimMouseBuffer() noexcept
{
   while (m_mouseBuffer.size() > m_bufferSize)
   {
      m_mouseBuffer.pop();
   }
}

void Input::trimCharBuffer() noexcept
{
   while (m_charBuffer.size() > m_bufferSize)
   {
      m_charBuffer.pop();
   }
}

void Input::trimKeyBoardBuffer() noexcept
{
   while (m_keyBoardBuffer.size() > m_bufferSize)
   {
      m_keyBoardBuffer.pop();
   }
}

void Input::trimRawBuffer() noexcept
{
   while (m_rawDeltaBuffer.size() > m_bufferSize)
   {
      m_rawDeltaBuffer.pop();
   }
}

void Input::onWheelDelta(int delta) noexcept
{
   m_wheelDeltaCarry += delta;
   //Generate for every 120
   while (m_wheelDeltaCarry >= WHEEL_DELTA)
   {
      m_wheelDeltaCarry -= WHEEL_DELTA;
      onWheelUp();
   }
   while (m_wheelDeltaCarry <= -WHEEL_DELTA)
   {
      m_wheelDeltaCarry += WHEEL_DELTA;
      onWheelDown();
   }
}
