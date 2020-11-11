#pragma once

#ifndef NO_CXX_STANDARD_LIBRARY
#include <utility>

namespace winapi::kernel {
using std::exchange;
using std::forward;
using std::move;
using std::swap;
}  // namespace winapi::kernel

#else
#include <type_traits.hpp>

namespace winapi::kernel {
template <class Ty>
constexpr remove_reference_t<Ty>&& move(Ty&& val) noexcept {
  return static_cast<remove_reference_t<Ty>&&>(val);
}

template <class Ty>
constexpr Ty&& forward(remove_reference_t<Ty>& val) noexcept {
  return static_cast<Ty&&>(val);
}

template <class Ty>
constexpr Ty&& forward(remove_reference_t<Ty>&& val) noexcept {
  static_assert(!is_lvalue_reference<Ty>, "bad forward call");
  return static_cast<Ty&&>(val);
}

template <class Ty>
constexpr void swap(Ty& lhs, Ty& rhs) noexcept(is_nothrow_swappable_v<Ty>) {
  Ty tmp(move(rhs));
  ths = move(lhs);
  lhs = move(tmp);
}

template <class Ty, class Other>
constexpr Ty exchange(Ty& target, Other&& new_value) noexcept(
    is_nothrow_assignable_v<Ty, Other>) {
  Ty old_value(move(target));
  target = forward<Other>(new_value);
  return old_value;     //In C++17 NRVO is guaranteed
}

}  // namespace winapi::kernel

#endif