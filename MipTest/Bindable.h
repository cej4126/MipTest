#pragma once
#include "Graphics.h"

namespace Bind
{
   class Bindable
   {
   public:
      virtual void draw() noexcept = 0;
      virtual ~Bindable() = default;
      virtual std::string getUID() const noexcept { return ""; }
      bool isInitialized() { return m_initualized; }
      void setInitialized() { m_initualized = true; }
      void setIndex(int index) { m_index = index; }
      int getIndex() { return m_index; }
   private:
      bool m_initualized = false;
      int m_index = -1;
   };

   class BindableCodex
   {
   public:
      template<class T, typename ... Params> static std::shared_ptr<T> resolve(Graphics &gfx, Params && ... p) noexcept
      {
         return get().resolveCodex<T>(gfx, std::forward < Params>(p) ...);
      }
   private:
      template<class T, typename ... Params> std::shared_ptr<T> resolveCodex(Graphics &gfx, Params && ... p) noexcept
      {
         const auto key = T::generateUID(std::forward<Params>(p) ...);
         const auto i = binds.find(key);
         if (i == binds.end())
         {
            auto bind = std::make_shared<T>(gfx, std::forward<Params>(p) ...);
            binds[key] = bind;
            return bind;
         }
         else
         {
            return std::static_pointer_cast<T>(i->second);
         }
      }

      static BindableCodex &get()
      {
         static BindableCodex codex;
         return codex;
      }
   private:
      std::unordered_map<std::string, std::shared_ptr<Bindable>> binds;
   };
}

