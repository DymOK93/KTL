#pragma once
#include <heap.h>
#include <memory_tools.hpp>
#include <smart_pointer.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

namespace ktl {
template <class Ty>
[[nodiscard]] constexpr Ty* launder(Ty* ptr) noexcept {
  static_assert(!is_function_v<Ty> && !is_void_v<Ty>,
                "N4727 21.6.4 [ptr.launder]/3: The program is ill-formed if Ty "
                "is a function type or cv void");
  return __builtin_launder(ptr);
}
}  // namespace ktl
