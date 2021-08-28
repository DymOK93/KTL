#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <memory>
#include <utility>
#else
#include <placement_new.hpp>
#include <type_traits.hpp>
#include <utility.hpp>
#endif

namespace ktl {
#ifndef KTL_NO_CXX_STANDARD_LIBRARY
// there is no std::construct_at in C++17
using std::destroy_at;
#else

template <class Ty, class... Types>
constexpr Ty* construct_at(Ty* place, Types&&... args) noexcept(
    is_nothrow_constructible_v<Ty, Types...>) {
  return new (const_cast<void*>(static_cast<const volatile void*>(place)))
      Ty(forward<Types>(args)...);
}

template <class Ty>
constexpr Ty* default_construct_at(Ty* place) noexcept(
    is_nothrow_default_constructible_v<Ty>) {
  return new (const_cast<void*>(static_cast<const volatile void*>(place))) Ty;
}

template <class Ty>
constexpr void destroy_at(
    Ty* object_ptr) noexcept {  // CRASH if d-tor isn't noexcept
  object_ptr->~Ty();
}
#endif
}  // namespace ktl
