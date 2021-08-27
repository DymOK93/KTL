#pragma once
#include <crt_assert.h>
#include <type_traits.hpp>

namespace ktl {
template <typename Ty, enable_if_t<is_integral_v<Ty>, int> = 0>
bool isnan(Ty) {
  return false;
}

template <typename Ty, enable_if_t<is_floating_point_v<Ty>, int> = 0>
bool isnan(Ty) {
  assert_with_msg(false, "floating point isn't supported");
  return false;
}

template <typename Ty, enable_if_t<is_integral_v<Ty>, int> = 0>
bool isinf(Ty) {
  return false;
}

template <typename Ty, enable_if_t<is_floating_point_v<Ty>, int> = 0>
bool isinf(Ty) {
  assert_with_msg(false, "floating point isn't supported");
  return false;
}

template <typename Ty, enable_if_t<is_integral_v<Ty>, int> = 0>
bool isfinite(Ty) {
  return true;
}

template <typename Ty, enable_if_t<is_floating_point_v<Ty>, int> = 0>
bool isfinite(Ty) {
  assert_with_msg(false, "floating point isn't supported");
  return false;
}

template <typename Ty, enable_if_t<is_floating_point_v<Ty>, int> = 0>
bool signbit(Ty) {
  assert_with_msg(false, "floating point isn't supported");
  return false;
}
}  // namespace ktl