#pragma once
#include <basic_types.h>
#include <limits.hpp>
#include <type_traits.hpp>

namespace ktl {
namespace rat::details {
template <intmax_t Value>
struct abs : integral_constant<intmax_t, (Value < 0 ? -Value : Value)> {
};  // computes absolute value of Value
}  // namespace rat::details

template <intmax_t Lhs,
          intmax_t Rhs,
          bool Sfinae = false,
          bool = (rat::details::abs<Lhs>::value <=
                  (numeric_limits<intmax_t>::max)() /
                      (Rhs == 0 ? 1 : rat::details::abs<Rhs>::value))>
struct safe_multiply : integral_constant<intmax_t, Lhs * Rhs> {
};  // Computes Lhs * Rhs without overflow

template <intmax_t Lhs, intmax_t Rhs, bool Sfinae>
struct safe_multiply<Lhs, Rhs, Sfinae, false> {  // Lhs * Rhs would overflow
  static_assert(Sfinae, "multiplication results in an integer overflow");
};

template <intmax_t Value>
struct sign_of : integral_constant<intmax_t, (Value < 0 ? -1 : 1)> {
};  // computes sign of Value

namespace rat::details {
template <intmax_t Lhs, intmax_t Rhs, bool OneSign, bool>
struct safe_add_impl : integral_constant<intmax_t, Lhs + Rhs> {
};  // computes Lhs + Rhs without overflow

template <intmax_t Lhs, intmax_t Rhs>
struct safe_add_impl<Lhs, Rhs, false, false> {
  static_assert(always_false_v<safe_add_impl>,
                "addition  results in an integer overflow");
};
}  // namespace rat::details

template <intmax_t Lhs, intmax_t Rhs>
struct safe_add
    : rat::details::safe_add_impl<Lhs,
                                  Rhs,
                                  sign_of<Lhs>::value != sign_of<Rhs>::value,
                                  (rat::details::abs<Lhs>::value <=
                                   (numeric_limits<intmax_t>::max)() -
                                       rat::details::abs<Rhs>::value)>::type {};

namespace rat::details {
template <intmax_t Lhs, intmax_t Rhs>
struct gcd_impl : gcd_impl<Rhs, Lhs % Rhs>::type {
};  // computes greatest common divisor of Lhs and Rhs

template <intmax_t Lhs>
struct gcd_impl<Lhs, 0> : integral_constant<intmax_t, Lhs> {
};  // computes GCD of Lhs and 0
}  // namespace rat::details

template <intmax_t Lhs, intmax_t Rhs>
struct gcd : rat::details::gcd_impl<rat::details::abs<Lhs>::value,
                                    rat::details::abs<Rhs>::value>::type {
};  // computes GCD of abs(Lhs) and abs(Rhs)

template <>
struct gcd<0, 0> : integral_constant<intmax_t, 1> {};

template <intmax_t Nx, intmax_t Dx = 1>
struct ratio {  // holds the ratio of Nx to Dx
  static_assert(Dx != 0, "zero denominator");
  static_assert(-(numeric_limits<intmax_t>::max)() <= Nx,
                "numerator too negative");
  static_assert(-(numeric_limits<intmax_t>::max)() <= Dx,
                "denominator too negative");

  static constexpr intmax_t num = sign_of<Nx>::value * sign_of<Dx>::value *
                                  rat::details::abs<Nx>::value /
                                  gcd<Nx, Dx>::value;

  static constexpr intmax_t den =
      rat::details::abs<Dx>::value / gcd<Nx, Dx>::value;

  using type = ratio<num, den>;
};

template <class Ty>
inline constexpr bool is_ratio_v = false;  // test for ratio type

template <intmax_t RxLhs, intmax_t RxRhs>
inline constexpr bool is_ratio_v<ratio<RxLhs, RxRhs>> = true;

// ALIAS TEMPLATE ratio_add
template <class RxLhs, class RxRhs>
struct ratio_add_impl {  // add two ratios
  static_assert(is_ratio_v<RxLhs> && is_ratio_v<RxRhs>,
                "ratio_add<R1, R2> requires R1 and R2 to be ratio<>s.");

  static constexpr intmax_t NxLhs = RxLhs::num;
  static constexpr intmax_t DxLhs = RxLhs::den;
  static constexpr intmax_t NxRhs = RxRhs::num;
  static constexpr intmax_t DxRhs = RxRhs::den;

  static constexpr intmax_t _Gx = _Gcd<DxLhs, DxRhs>::value;

  // typename ratio<>::type is necessary here
  using type =
      typename ratio<safe_add<safe_multiply<NxLhs, DxRhs / _Gx>::value,
                              safe_multiply<NxRhs, DxLhs / _Gx>::value>::value,
                     safe_multiply<DxLhs, DxRhs / _Gx>::value>::type;
};

template <class RxLhs, class RxRhs>
using ratio_add = typename ratio_add_impl<RxLhs, RxRhs>::type;

// ALIAS TEMPLATE ratio_subtract
template <class RxLhs, class RxRhs>
struct ratio_subtract_impl {  // subtract two ratios
  static_assert(is_ratio_v<RxLhs> && is_ratio_v<RxRhs>,
                "ratio_subtract<R1, R2> requires R1 and R2 to be ratio<>s.");

  static constexpr intmax_t NxRhs = RxRhs::num;
  static constexpr intmax_t DxRhs = RxRhs::den;

  using type = ratio_add<RxLhs, ratio<-NxRhs, DxRhs>>;
};

template <class RxLhs, class RxRhs>
using ratio_subtract = typename ratio_subtract_impl<RxLhs, RxRhs>::type;

// ALIAS TEMPLATE ratio_multiply
template <class RxLhs, class RxRhs>
struct ratio_multiply_impl {  // multiply two ratios
  static_assert(is_ratio_v<RxLhs> && is_ratio_v<RxRhs>,
                "ratio_multiply<R1, R2> requires R1 and R2 to be ratio<>s.");

  static constexpr intmax_t NxLhs = RxLhs::num;
  static constexpr intmax_t DxLhs = RxLhs::den;
  static constexpr intmax_t NxRhs = RxRhs::num;
  static constexpr intmax_t DxRhs = RxRhs::den;

  static constexpr intmax_t Gx = gcd<NxLhs, DxRhs>::value;
  static constexpr intmax_t Gy = gcd<NxRhs, DxLhs>::value;

  using numerator = safe_multiply<NxLhs / Gx, NxRhs / Gy, true>;
  using denominator = safe_multiply<DxLhs / Gy, DxRhs / Gx, true>;
};

template <class RxLhs, class RxRhs, bool Sfinae = true, class = void>
struct ratio_multiply_impl_sfinae {  // detect overflow during multiplication
  static_assert(Sfinae, "integer arithmetic overflow");
};

template <class RxLhs, class RxRhs, bool Sfinae>
struct ratio_multiply_impl_sfinae<
    RxLhs,
    RxRhs,
    Sfinae,
    void_t<typename ratio_multiply_impl<RxLhs, RxRhs>::numerator::type,
           typename ratio_multiply_impl<RxLhs, RxRhs>::denominator::
               type>> {  // typename ratio<>::type is unnecessary here
  using type = ratio<ratio_multiply_impl<RxLhs, RxRhs>::numerator::value,
                     ratio_multiply_impl<RxLhs, RxRhs>::denominator::value>;
};

template <class RxLhs, class RxRhs>
using ratio_multiply =
    typename ratio_multiply_impl_sfinae<RxLhs, RxRhs, false>::type;

// ALIAS TEMPLATE ratio_divide
template <class RxLhs, class RxRhs>
struct ratio_divide_impl {  // divide two ratios
  static_assert(is_ratio_v<RxLhs> && is_ratio_v<RxRhs>,
                "ratio_divide<R1, R2> requires R1 and R2 to be ratio<>s.");

  static constexpr intmax_t NxRhs = RxRhs::num;
  static constexpr intmax_t DxRhs = RxRhs::den;

  using RxRhs_inverse = ratio<DxRhs, NxRhs>;
};

template <class RxLhs, class RxRhs, bool Sfinae = true>
using ratio_divide_sfinae = typename ratio_multiply_impl_sfinae<
    RxLhs,
    typename ratio_divide_impl<RxLhs, RxRhs>::RxRhs_inverse,
    Sfinae>::type;

template <class RxLhs, class RxRhs>
using ratio_divide = ratio_divide_sfinae<RxLhs, RxRhs, false>;

// STRUCT TEMPLATE ratio_equal
template <class RxLhs, class RxRhs>
struct ratio_equal
    : bool_constant<RxLhs::num == RxRhs::num &&
                    RxLhs::den == RxRhs::den> {  // tests if ratio == ratio
  static_assert(is_ratio_v<RxLhs> && is_ratio_v<RxRhs>,
                "ratio_equal<R1, R2> requires R1 and R2 to be ratio<>s.");
};

template <class RxLhs, class RxRhs>
inline constexpr bool ratio_equal_v = ratio_equal<RxLhs, RxRhs>::value;

// STRUCT TEMPLATE ratio_not_equal
template <class RxLhs, class RxRhs>
struct ratio_not_equal
    : bool_constant<!ratio_equal_v<RxLhs, RxRhs>> {  // tests if ratio != ratio
  static_assert(is_ratio_v<RxLhs> && is_ratio_v<RxRhs>,
                "ratio_not_equal<R1, R2> requires R1 and R2 to be ratio<>s.");
};

template <class RxLhs, class RxRhs>
inline constexpr bool ratio_not_equal_v = ratio_not_equal<RxLhs, RxRhs>::value;

// STRUCT TEMPLATE ratio_less
struct uint128 {
  uint64_t upper;
  uint64_t lower;

  constexpr bool operator<(const uint128 rhs) const noexcept {
    if (upper != rhs.upper) {
      return upper < rhs.upper;
    }

    return lower < rhs.lower;
  }
};

constexpr uint128 multiply_uint128(
    const uint64_t lhs_factor,
    const uint64_t
        rhs_factor) noexcept {  // multiply two 64-bit integers into a
                                // 128-bit integer, Knuth's algorithm M
  const uint64_t _Llow = lhs_factor & 0xFFFF'FFFFULL;
  const uint64_t _Lhigh = lhs_factor >> 32;
  const uint64_t _Rlow = rhs_factor & 0xFFFF'FFFFULL;
  const uint64_t _Rhigh = rhs_factor >> 32;

  uint64_t _Temp = _Llow * _Rlow;
  const uint64_t lower32 = _Temp & 0xFFFF'FFFFULL;
  uint64_t _Carry = _Temp >> 32;

  _Temp = _Llow * _Rhigh + _Carry;
  const uint64_t _Mid_lower = _Temp & 0xFFFF'FFFFULL;
  const uint64_t _Mid_uint128 = _Temp >> 32;

  _Temp = _Lhigh * _Rlow + _Mid_lower;
  _Carry = _Temp >> 32;

  return {_Lhigh * _Rhigh + _Mid_uint128 + _Carry, (_Temp << 32) + lower32};
}

constexpr bool is_ratio_less(const int64_t NxLhs,
                             const int64_t DxLhs,
                             const int64_t NxRhs,
                             const int64_t DxRhs) noexcept {
  if (NxLhs >= 0 && NxRhs >= 0) {
    return multiply_uint128(static_cast<uint64_t>(NxLhs),
                            static_cast<uint64_t>(DxRhs)) <
           multiply_uint128(static_cast<uint64_t>(NxRhs),
                            static_cast<uint64_t>(DxLhs));
  }

  if (NxLhs < 0 && NxRhs < 0) {
    return multiply_uint128(static_cast<uint64_t>(-NxRhs),
                            static_cast<uint64_t>(DxLhs)) <
           multiply_uint128(static_cast<uint64_t>(-NxLhs),
                            static_cast<uint64_t>(DxRhs));
  }

  return NxLhs < NxRhs;
}

template <class RxLhs, class RxRhs>
struct ratio_less
    : bool_constant<is_ratio_less(RxLhs::num,
                                  RxLhs::den,
                                  RxRhs::num,
                                  RxRhs::den)> {  // tests if ratio < ratio
  static_assert(is_ratio_v<RxLhs> && is_ratio_v<RxRhs>,
                "ratio_less<R1, R2> requires R1 and R2 to be ratio<>s.");
};

template <class RxLhs, class RxRhs>
inline constexpr bool ratio_less_v = ratio_less<RxLhs, RxRhs>::value;

template <class RxLhs, class RxRhs>
struct ratio_less_equal
    : bool_constant<!ratio_less_v<RxRhs, RxLhs>> {  // tests if ratio <= ratio
  static_assert(is_ratio_v<RxLhs> && is_ratio_v<RxRhs>,
                "ratio_less_equal<R1, R2> requires R1 and R2 to be ratio<>s.");
};

template <class RxLhs, class RxRhs>
inline constexpr bool ratio_less_equal_v =
    ratio_less_equal<RxLhs, RxRhs>::value;

template <class RxLhs, class RxRhs>
struct ratio_greater
    : ratio_less<RxRhs, RxLhs>::type {  // tests if ratio > ratio
  static_assert(is_ratio_v<RxLhs> && is_ratio_v<RxRhs>,
                "ratio_greater<R1, R2> requires R1 and R2 to be ratio<>s.");
};

template <class RxLhs, class RxRhs>
inline constexpr bool ratio_greater_v = ratio_greater<RxLhs, RxRhs>::value;

template <class RxLhs, class RxRhs>
struct ratio_greater_equal
    : bool_constant<!ratio_less_v<RxLhs, RxRhs>> {  // tests if ratio >= ratio
  static_assert(
      is_ratio_v<RxLhs> && is_ratio_v<RxRhs>,
      "ratio_greater_equal<R1, R2> requires R1 and R2 to be ratio<>s.");
};

template <class RxLhs, class RxRhs>
inline constexpr bool ratio_greater_equal_v =
    ratio_greater_equal<RxLhs, RxRhs>::value;

namespace rat::details {
constexpr intmax_t pow10(uint8_t power) noexcept {
  intmax_t result{1};
  for (size_t idx = 0; idx < power; ++idx) {
    result *= 10;
  }
  return result;
}
}  // namespace rat::details

using atto = ratio<1, rat::details::pow10(18)>;
using femto = ratio<1, rat::details::pow10(15)>;
using pico = ratio<1, rat::details::pow10(12)>;
using nano = ratio<1, rat::details::pow10(9)>;
using micro = ratio<1, rat::details::pow10(6)>;
using milli = ratio<1, rat::details::pow10(3)>;
using centi = ratio<1, rat::details::pow10(2)>;
using deci = ratio<1, rat::details::pow10(1)>;
using deca = ratio<rat::details::pow10(1), 1>;
using hecto = ratio<rat::details::pow10(2), 1>;
using kilo = ratio<rat::details::pow10(3), 1>;
using mega = ratio<rat::details::pow10(6), 1>;
using giga = ratio<rat::details::pow10(9), 1>;
using tera = ratio<rat::details::pow10(12), 1>;
using peta = ratio<rat::details::pow10(15), 1>;
using exa = ratio<rat::details::pow10(18), 1>;
}  // namespace ktl