#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <utility>

namespace ktl {
using std::addressof;
using std::exchange;
using std::forward;
using std::move;
using std::swap;
}  // namespace ktl

#else
#include <intrinsic.hpp>
#include <type_traits_impl.hpp>

namespace ktl {
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

struct in_place_t {
  explicit in_place_t() = default;
};
inline constexpr in_place_t in_place{};

template <class Ty>
struct in_place_type_t {
  explicit in_place_type_t() noexcept = default;
};

template <class Ty>
inline constexpr in_place_type_t<Ty> in_place_type{};

template <size_t Idx>
struct in_place_index_t {
  explicit in_place_index_t() noexcept = default;
};

template <size_t Idx>
inline constexpr in_place_index_t<Idx> in_place_index{};
}  // namespace ktl
#endif