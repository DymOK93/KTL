#pragma once

namespace ktl {
enum float_round_style {
  round_indeterminate = -1,
  round_toward_zero = 0,
  round_to_nearest = 1,
  round_toward_infinity = 2,
  round_toward_neg_infinity = 3
};

template <class Ty>
struct numeric_limits;

template <>
struct numeric_limits<bool>;

template <>
struct numeric_limits<char>;

template <>
struct numeric_limits<signed char>;

template <>
struct numeric_limits<unsigned char>;

template <>
struct numeric_limits<wchar_t>;

template <>
struct numeric_limits<char16_t>;

template <>
struct numeric_limits<char32_t>;

template <>
struct numeric_limits<short>;

template <>
struct numeric_limits<unsigned short>;

template <>
struct numeric_limits<int>;

template <>
struct numeric_limits<unsigned int> {
  static constexpr bool is_specialized = true;
  static constexpr bool is_signed = false;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm = false;
  static constexpr bool has_denorm_loss = false;
  static constexpr float_round_style round_style = round_toward_zero;
  static constexpr bool is_iec599 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = true;
  static constexpr bool traps = true;
  static constexpr bool tinyness_before = false;

  static constexpr unsigned int(min)() noexcept { return 0; };
  static constexpr unsigned int lowest() noexcept { return (min)(); };
  static constexpr unsigned int(max)() noexcept { return 0xffff'ffff; };
};

template <>
struct numeric_limits<long>;

template <>
struct numeric_limits<unsigned long>;

template <>
struct numeric_limits<long long> {
  static constexpr bool is_specialized = true;
  static constexpr bool is_signed = true;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm = false;
  static constexpr bool has_denorm_loss = false;
  static constexpr float_round_style round_style = round_toward_zero;
  static constexpr bool is_iec599 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = false;
  static constexpr bool traps = true;
  static constexpr bool tinyness_before = false;

  static constexpr long long(min)() noexcept {
    return -9223372036854775807ll - 1;
  };
  static constexpr long long lowest() noexcept { return (min)(); };
  static constexpr long long(max)() noexcept { return 9223372036854775807ll; };
};

template <>
struct numeric_limits<unsigned long long>;

template <>
struct numeric_limits<float>;

template <>
struct numeric_limits<double>;

template <>
struct numeric_limits<long double>;
}  // namespace ktl