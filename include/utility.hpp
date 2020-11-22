#pragma once

#ifndef NO_CXX_STANDARD_LIBRARY
#include <utility>

namespace winapi::kernel {
using std::exchange;
using std::forward;
using std::move;
using std::swap;
using std::addressof;
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
  static_assert(!is_lvalue_reference_v<Ty>, "bad forward call");
  return static_cast<Ty&&>(val);
}

template <class Ty>
constexpr void swap(Ty& lhs, Ty& rhs) noexcept(is_nothrow_swappable_v<Ty>) {
  Ty tmp(move(rhs));
  rhs = move(lhs);
  lhs = move(tmp);
}

template <class Ty, class Other>
constexpr Ty exchange(Ty& target, Other&& new_value) noexcept(
    is_nothrow_move_constructible_v<Ty>&&
        is_nothrow_assignable_v<add_lvalue_reference_t<Ty>, Other>) {
  Ty old_value(move(target));
  target = forward<Other>(new_value);
  return old_value;  // In C++17 NRVO is guaranteed
}

template <class Ty>
constexpr Ty* addressof(Ty& target) noexcept {
  return __builtin_addressof(target);
}

template <class Ty>
const Ty* addressof(const Ty&&) = delete;

template <class Ty>
constexpr const Ty* as_const(Ty* ptr) {
  return static_cast<add_pointer_t<add_const_t<Ty> > >(ptr);
}

template <class Ty>
constexpr const Ty& as_const(Ty& val) {
  return static_cast<add_lvalue_reference_t<add_const_t<Ty> > >(val);
}

template <class Ty, class U>
constexpr auto* pointer_cast(U* ptr) {
  using pvoid_type = conditional_t<is_const_v<U>, const void*, void*>;
  using pointer_type =
      conditional_t<is_const_v<U>, add_pointer_t<add_const_t<Ty> >,
                    add_pointer_t<Ty> >;

  return static_cast<pointer_type>(static_cast<pvoid_type>(ptr));
}
}  // namespace winapi::kernel
#endif