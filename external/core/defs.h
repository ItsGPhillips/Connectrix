#pragma once

#include <assert.h>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <vector>
#include <optional>

#include <absl/container/flat_hash_map.h>

#include "debug_break.h"

namespace core
{
   using i8 = int8_t;
   using i16 = int16_t;
   using i32 = int32_t;
   using i64 = int64_t;

   using u8 = uint8_t;
   using u16 = uint16_t;
   using u32 = uint32_t;
   using u64 = uint64_t;

   using f32 = float;
   using f64 = double;

   using usize = size_t;

   // ======================================================================

   // Alias of unique_ptr
   template <class T>
   using Box = std::unique_ptr<T>;

   // Alias of shared_ptr
   template <class T>
   using Rc = std::shared_ptr<T>;

   // Alias of atomic<shared_ptr<...>>
   template <class T>
   using Arc = std::atomic<std::shared_ptr<T>>;

   // Alias of std::vector<...>;
   template <class T>
   using Vec = std::vector<T>;

   template <class K, class T>
   using HashMap = absl::flat_hash_map<K, T>;

   template<class T>
   using Option = std::optional<T>;
   using None   = std::nullopt_t;

} // namespace core

namespace core
{
#ifdef __GNUC__ // GCC 4.8+, Clang, Intel and other compilers compatible with GCC (-std=c++0x or above)
   [[noreturn]] inline __attribute__ ((always_inline)) void unreachable ()
   {
      __builtin_unreachable ();
   }
#elif defined(_MSC_VER) // MSVC

   [[noreturn]] __forceinline void unreachable () { __assume (false); }
#else                   // ???
   inline void unreachable () {}
#endif

#define FORCE_MACRO_SEMICOLON(macro_inner)                                                         \
   do {                                                                                            \
      macro_inner                                                                                  \
   } while (false)

#ifdef NDEBUG
   // If this macro allows the compiler to elimiate unused branches that the user can
   // guarantee are never reached. Be very careful when using this macro as it will cause UB
   // if reached.
   // In debug mode this will trigger a breakpoint if reached
   #define UNREACHABLE ::core::unreachable ();
   #define ASSERT_FALSE(message) FORCE_MACRO_SEMICOLON ()
#else
   // If this macro allows the compiler to elimiate unused branches that the user can
   // guarantee are never reached. Be very careful when using this macro as it will cause UB
   // if reached.
   // In debug mode this will trigger a breakpoint if reached
   #define UNREACHABLE debug_break;
   #define ASSERT_FALSE(message) FORCE_MACRO_SEMICOLON (assert (false);)
#endif

   /*
      Templated struct that auto inherits the Ts::operator() overload from
      any number of passed in types, thus creating one master type that has the
      inherited functionality of all passed in types.
      This is used in std::visit(...) as  easy way to create a visitor.
   */
   template <class... Ts>
   struct overloaded: Ts...
   {
      using Ts::operator()...;
   };

   /*
      Templated Constructor for the overloaded struct allowing the template types
      to be seen by the caller.
   */
   template <class... Ts>
   overloaded (Ts...) -> overloaded<Ts...>;

#if _MSC_VER
   #define NO_RETURN __declspec(noreturn)
#endif

#define FATAL_ERROR debug_abort ()

#define await co_await
#define loop while (true)

} // namespace core