#pragma once
#include "stdafx.h"

class Input
{
   friend class Window;

public:
   struct RawDelta
   {
      int x;
      int y;
   };

   class MouseEvent
   {
   public:
      enum class MouseType
      {
         LeftPress,
         LeftRelease,
         RightPress,
         RightRelease,
         WheelUp,
         WheelDown,
         MouseMove,
         EnterWindow,
         LeaveWindow
      };
      MouseEvent(MouseType type, const Input &parent) noexcept
         :
         m_type(type),
         m_leftIsPressed(parent.m_leftIsPressed),
         m_rightIsPressed(parent.m_rightIsPressed),
         m_x(parent.m_x),
         m_y(parent.m_y)
      {
      }
      
   private:
      MouseType m_type;
      bool m_leftIsPressed;
      bool m_rightIsPressed;
      int m_x;
      int m_y;
   };

   class KeyBoardEvent
   {
   public:
      enum class KeyBoardType
      {
         KeyBoardPress,
         KeyBoardRelease,
      };
      KeyBoardEvent(KeyBoardType type, unsigned char code) noexcept
         :
         m_type(type),
         m_code(code)
      {
      }

      KeyBoardType getType() const noexcept { return m_type; }
      bool isPress() { return (m_type == KeyBoardType::KeyBoardPress); }
      bool isRelease() { return (m_type == KeyBoardType::KeyBoardRelease); }
      int getCode() { return m_code; }

   private:
      KeyBoardType m_type;
      unsigned char m_code;
   };


public:
   Input() = default;
   Input(const Input &) = delete;
   Input &operator=(const Input &) = delete;

   // Mouse Functions
   bool isLeftPressed() const noexcept { return m_leftIsPressed; }
   bool isRrightPressed() const noexcept { return m_rightIsPressed; }
   std::optional<RawDelta> readRawDelta() noexcept;
   bool isInWindow() { return m_isInWindow; }
   void enableRaw() noexcept { m_rawEnable = true; }
   void disableRaw() noexcept { m_rawEnable = false; }
   bool isRawEnabled() noexcept { return m_rawEnable; }

   // Ket Functions
   bool isKeyPressed(unsigned char code) const noexcept { return m_keyStates[code]; }
   std::optional<KeyBoardEvent> readKey() noexcept;

   void enableAutoRepeat() noexcept { m_autoRepeatEnable = true; }
   void disableAutoRepeat() noexcept { m_autoRepeatEnable = false; }
   bool isAutoRepeatEnable() const noexcept { return m_autoRepeatEnable; }

private:
   void onKeyBoardPressed(unsigned char code) noexcept;
   void onKeyBoardRelease(unsigned char code) noexcept;
   void onChar(char character) noexcept;
   void clear() noexcept;
   void onMouseMove(int x, int y) noexcept;
   void onMouseLeave() noexcept;
   void onMouseEnter() noexcept;
   void onRawDelta(int dx, int dy) noexcept;
   void onLeftPressed() noexcept;
   void onLeftReleased() noexcept;
   void onRightPressed() noexcept;
   void onRightReleased() noexcept;

   void onWheelUp() noexcept;
   void onWheelDown() noexcept;
   void onWheelDelta(int delta) noexcept;

   void trimMouseBuffer() noexcept;
   void trimCharBuffer() noexcept;
   void trimKeyBoardBuffer() noexcept;
   void trimRawBuffer() noexcept;

   static constexpr unsigned int m_bufferSize = 16u;
   int m_x;
   int m_y;
   bool m_leftIsPressed = false;
   bool m_rightIsPressed = false;
   bool m_isInWindow = false;
   int m_wheelDeltaCarry = 0;
   std::queue <MouseEvent> m_mouseBuffer;

   static constexpr unsigned int nKeys = 256u;
   bool m_autoRepeatEnable = false;
   bool m_rawEnable = false;
   std::bitset<nKeys> m_keyStates;
   std::queue<KeyBoardEvent> m_keyBoardBuffer;
   std::queue<char> m_charBuffer;
   std::queue<RawDelta> m_rawDeltaBuffer;
};

