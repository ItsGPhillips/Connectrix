#pragma once
#include "../defs.h"

namespace core::containers
{

   template <class T>
   class Slab
   {
   public:
      Slab () = default;
      Slab (usize initalCapacity);

   private:
      using Slot = std::variant<std::monostate, T>
      usize insertIndex = 0;
      Vec<Slot>;
   };

} // namespace core::containers