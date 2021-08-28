#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <limits>
namespace ktl {
using std::numeric_limits;
}
#else
#include <float.h>
#include <limits.h>
#include <wchar.h>

// workaround for kernel-mode version of <float.hpp>
#ifndef FLT_TRUE_MIN
#undef FLT_TRUE_MIN
#define FLT_TRUE_MIN 1.401298464e-45F  // min positive value
#endif

#ifndef DBL_TRUE_MIN
#undef DBL_TRUE_MIN
#define DBL_TRUE_MIN 4.9406564584124654e-324  // min positive value
#endif

#ifndef LDBL_TRUE_MIN
#undef LDBL_TRUE_MIN
#define LDBL_TRUE_MIN DBL_TRUE_MIN  // min positive value
#endif

namespace ktl {
enum float_denorm_style {  // constants for different IEEE float denormalization
                           // styles
  denorm_indeterminate = -1,
  denorm_absent = 0,
  denorm_present = 1
};

// ENUM float_round_style
enum float_round_style {  // constants for different IEEE rounding styles
  round_indeterminate = -1,
  round_toward_zero = 0,
  round_to_nearest = 1,
  round_toward_infinity = 2,
  round_toward_neg_infinity = 3
};

// STRUCT numeric_limit_base
struct numeric_limit_base {  // base for all types, with common defaults
  static constexpr float_denorm_style has_denorm = denorm_absent;
  static constexpr bool has_denorm_loss{false};
  static constexpr bool has_infinity{false};
  static constexpr bool has_quiet_NaN{false};
  static constexpr bool has_signaling_NaN{false};
  static constexpr bool is_bounded{false};
  static constexpr bool is_exact{false};
  static constexpr bool is_iec559{false};
  static constexpr bool is_integer{false};
  static constexpr bool is_modulo{false};
  static constexpr bool is_signed{false};
  static constexpr bool is_specialized{false};
  static constexpr bool tinyness_before{false};
  static constexpr bool traps{false};
  static constexpr float_round_style round_style{round_toward_zero};
  static constexpr int digits{0};
  static constexpr int digits10{0};
  static constexpr int max_digits10{0};
  static constexpr int max_exponent{0};
  static constexpr int max_exponent10{0};
  static constexpr int min_exponent{0};
  static constexpr int min_exponent10{0};
  static constexpr int radix{0};
};

// CLASS TEMPLATE numeric_limits
template <class Ty>
struct numeric_limits : numeric_limit_base {  // numeric limits for arbitrary
                                              // type Ty (say little or nothing)
  [[nodiscard]] static constexpr Ty(min)() noexcept { return Ty{}; }
  [[nodiscard]] static constexpr Ty(max)() noexcept { return Ty{}; }
  [[nodiscard]] static constexpr Ty lowest() noexcept { return Ty{}; }
  [[nodiscard]] static constexpr Ty epsilon() noexcept { return Ty{}; }
  [[nodiscard]] static constexpr Ty round_error() noexcept { return Ty{}; }
  [[nodiscard]] static constexpr Ty denorm_min() noexcept { return Ty{}; }
  [[nodiscard]] static constexpr Ty infinity() noexcept { return Ty{}; }
  [[nodiscard]] static constexpr Ty quiet_NaN() noexcept { return Ty{}; }
  [[nodiscard]] static constexpr Ty signaling_NaN() noexcept { return Ty{}; }
};

template <class Ty>
struct numeric_limits<const Ty> : public numeric_limits<Ty> {
};  // numeric limits for const types

template <class Ty>
struct numeric_limits<volatile Ty> : public numeric_limits<Ty> {
};  // numeric limits for volatile types

template <class Ty>
struct numeric_limits<const volatile Ty> : public numeric_limits<Ty> {
};  // numeric limits for const volatile types

struct numeric_limits_integral : numeric_limit_base {  // base for integer types
  static constexpr bool is_bounded = true;
  static constexpr bool is_exact = true;
  static constexpr bool is_integer = true;
  static constexpr bool is_specialized = true;
  static constexpr int radix = 2;
};

template <>
struct numeric_limits<bool> : public numeric_limits_integral {
  static constexpr int digits{1};

  [[nodiscard]] static constexpr bool(min)() noexcept { return false; }
  [[nodiscard]] static constexpr bool(max)() noexcept { return true; }
  [[nodiscard]] static constexpr bool lowest() noexcept { return (min)(); }
  [[nodiscard]] static constexpr bool epsilon() noexcept { return 0; }
  [[nodiscard]] static constexpr bool round_error() noexcept { return 0; }
  [[nodiscard]] static constexpr bool denorm_min() noexcept { return 0; }
  [[nodiscard]] static constexpr bool infinity() noexcept { return 0; }
  [[nodiscard]] static constexpr bool quiet_NaN() noexcept { return 0; }
  [[nodiscard]] static constexpr bool signaling_NaN() noexcept { return 0; }
};

template <>
struct numeric_limits<char> : public numeric_limits_integral {
  static constexpr bool is_signed{CHAR_MIN != 0};
  static constexpr bool is_modulo{CHAR_MIN == 0};
  static constexpr int digits{8 - is_signed};
  static constexpr int digits10{2};

  [[nodiscard]] static constexpr char(min)() noexcept { return CHAR_MIN; }
  [[nodiscard]] static constexpr char(max)() noexcept { return CHAR_MAX; }
  [[nodiscard]] static constexpr char lowest() noexcept { return (min)(); }
  [[nodiscard]] static constexpr char epsilon() noexcept { return 0; }
  [[nodiscard]] static constexpr char round_error() noexcept { return 0; }
  [[nodiscard]] static constexpr char denorm_min() noexcept { return 0; }
  [[nodiscard]] static constexpr char infinity() noexcept { return 0; }
  [[nodiscard]] static constexpr char quiet_NaN() noexcept { return 0; }
  [[nodiscard]] static constexpr char signaling_NaN() noexcept { return 0; }
};

template <>
struct numeric_limits<signed char> : public numeric_limits_integral {
  static constexpr bool is_signed{true};
  static constexpr int digits{7};
  static constexpr int digits10{2};

  [[nodiscard]] static constexpr signed char(min)() noexcept {
    return SCHAR_MIN;
  }
  [[nodiscard]] static constexpr signed char(max)() noexcept {
    return SCHAR_MAX;
  }
  [[nodiscard]] static constexpr signed char lowest() noexcept {
    return (min)();
  }

  [[nodiscard]] static constexpr signed char epsilon() noexcept { return 0; }

  [[nodiscard]] static constexpr signed char round_error() noexcept {
    return 0;
  }

  [[nodiscard]] static constexpr signed char denorm_min() noexcept { return 0; }
  [[nodiscard]] static constexpr signed char infinity() noexcept { return 0; }
  [[nodiscard]] static constexpr signed char quiet_NaN() noexcept { return 0; }

  [[nodiscard]] static constexpr signed char signaling_NaN() noexcept {
    return 0;
  }
};

template <>
struct numeric_limits<unsigned char> : public numeric_limits_integral {
  static constexpr bool is_modulo{true};
  static constexpr int digits{8};
  static constexpr int digits10{2};

  [[nodiscard]] static constexpr unsigned char(min)() noexcept { return 0; }

  [[nodiscard]] static constexpr unsigned char(max)() noexcept {
    return UCHAR_MAX;
  }
  [[nodiscard]] static constexpr unsigned char lowest() noexcept {
    return (min)();
  }

  [[nodiscard]] static constexpr unsigned char epsilon() noexcept { return 0; }

  [[nodiscard]] static constexpr unsigned char round_error() noexcept {
    return 0;
  }
  [[nodiscard]] static constexpr unsigned char denorm_min() noexcept {
    return 0;
  }

  [[nodiscard]] static constexpr unsigned char infinity() noexcept { return 0; }

  [[nodiscard]] static constexpr unsigned char quiet_NaN() noexcept {
    return 0;
  }
  [[nodiscard]] static constexpr unsigned char signaling_NaN() noexcept {
    return 0;
  }
};

template <>
struct numeric_limits<char16_t> : public numeric_limits_integral {
 public:
  static constexpr bool is_modulo{true};
  static constexpr int digits{16};
  static constexpr int digits10{4};

  [[nodiscard]] static constexpr char16_t(min)() noexcept { return 0; }
  [[nodiscard]] static constexpr char16_t(max)() noexcept { return USHRT_MAX; }
  [[nodiscard]] static constexpr char16_t lowest() noexcept { return (min)(); }
  [[nodiscard]] static constexpr char16_t epsilon() noexcept { return 0; }
  [[nodiscard]] static constexpr char16_t round_error() noexcept { return 0; }
  [[nodiscard]] static constexpr char16_t denorm_min() noexcept { return 0; }
  [[nodiscard]] static constexpr char16_t infinity() noexcept { return 0; }
  [[nodiscard]] static constexpr char16_t quiet_NaN() noexcept { return 0; }
  [[nodiscard]] static constexpr char16_t signaling_NaN() noexcept { return 0; }
};

template <>
struct numeric_limits<char32_t> : public numeric_limits_integral {
  static constexpr bool is_modulo{true};
  static constexpr int digits = 32;
  static constexpr int digits10 = 9;

  [[nodiscard]] static constexpr char32_t(min)() noexcept { return 0; }
  [[nodiscard]] static constexpr char32_t(max)() noexcept { return UINT_MAX; }
  [[nodiscard]] static constexpr char32_t lowest() noexcept { return (min)(); }
  [[nodiscard]] static constexpr char32_t epsilon() noexcept { return 0; }
  [[nodiscard]] static constexpr char32_t round_error() noexcept { return 0; }
  [[nodiscard]] static constexpr char32_t denorm_min() noexcept { return 0; }
  [[nodiscard]] static constexpr char32_t infinity() noexcept { return 0; }
  [[nodiscard]] static constexpr char32_t quiet_NaN() noexcept { return 0; }
  [[nodiscard]] static constexpr char32_t signaling_NaN() noexcept { return 0; }
};

template <>
struct numeric_limits<wchar_t> : public numeric_limits_integral {
 public:
  static constexpr bool is_modulo{true};
  static constexpr int digits{16};
  static constexpr int digits10{4};

  [[nodiscard]] static constexpr wchar_t(min)() noexcept { return WCHAR_MIN; }
  [[nodiscard]] static constexpr wchar_t(max)() noexcept { return WCHAR_MAX; }
  [[nodiscard]] static constexpr wchar_t lowest() noexcept { return (min)(); }
  [[nodiscard]] static constexpr wchar_t epsilon() noexcept { return 0; }
  [[nodiscard]] static constexpr wchar_t round_error() noexcept { return 0; }
  [[nodiscard]] static constexpr wchar_t denorm_min() noexcept { return 0; }
  [[nodiscard]] static constexpr wchar_t infinity() noexcept { return 0; }
  [[nodiscard]] static constexpr wchar_t quiet_NaN() noexcept { return 0; }
  [[nodiscard]] static constexpr wchar_t signaling_NaN() noexcept { return 0; }
};

template <>
struct numeric_limits<short> : public numeric_limits_integral {
 public:
  static constexpr bool is_signed{true};
  static constexpr int digits{15};
  static constexpr int digits10{4};

  [[nodiscard]] static constexpr short(min)() noexcept { return SHRT_MIN; }
  [[nodiscard]] static constexpr short(max)() noexcept { return SHRT_MAX; }
  [[nodiscard]] static constexpr short lowest() noexcept { return (min)(); }
  [[nodiscard]] static constexpr short epsilon() noexcept { return 0; }
  [[nodiscard]] static constexpr short round_error() noexcept { return 0; }
  [[nodiscard]] static constexpr short denorm_min() noexcept { return 0; }
  [[nodiscard]] static constexpr short infinity() noexcept { return 0; }
  [[nodiscard]] static constexpr short quiet_NaN() noexcept { return 0; }
  [[nodiscard]] static constexpr short signaling_NaN() noexcept { return 0; }
};

template <>
struct numeric_limits<int> : public numeric_limits_integral {
  static constexpr bool is_signed{true};
  static constexpr int digits{31};
  static constexpr int digits10{9};

  [[nodiscard]] static constexpr int(min)() noexcept { return INT_MIN; }
  [[nodiscard]] static constexpr int(max)() noexcept { return INT_MAX; }
  [[nodiscard]] static constexpr int lowest() noexcept { return (min)(); }
  [[nodiscard]] static constexpr int epsilon() noexcept { return 0; }
  [[nodiscard]] static constexpr int round_error() noexcept { return 0; }
  [[nodiscard]] static constexpr int denorm_min() noexcept { return 0; }
  [[nodiscard]] static constexpr int infinity() noexcept { return 0; }
  [[nodiscard]] static constexpr int quiet_NaN() noexcept { return 0; }
  [[nodiscard]] static constexpr int signaling_NaN() noexcept { return 0; }
};

template <>
struct numeric_limits<long> : public numeric_limits_integral {
  static_assert(sizeof(int) == sizeof(long), "LLP64 assumption");
  static constexpr bool is_signed{true};
  static constexpr int digits{31};
  static constexpr int digits10{9};

  [[nodiscard]] static constexpr long(min)() noexcept { return LONG_MIN; }
  [[nodiscard]] static constexpr long(max)() noexcept { return LONG_MAX; }
  [[nodiscard]] static constexpr long lowest() noexcept { return (min)(); }
  [[nodiscard]] static constexpr long epsilon() noexcept { return 0; }
  [[nodiscard]] static constexpr long round_error() noexcept { return 0; }
  [[nodiscard]] static constexpr long denorm_min() noexcept { return 0; }
  [[nodiscard]] static constexpr long infinity() noexcept { return 0; }
  [[nodiscard]] static constexpr long quiet_NaN() noexcept { return 0; }
  [[nodiscard]] static constexpr long signaling_NaN() noexcept { return 0; }
};

template <>
struct numeric_limits<long long> : public numeric_limits_integral {
  static constexpr bool is_signed{true};
  static constexpr int digits{63};
  static constexpr int digits10{18};

  [[nodiscard]] static constexpr long long(min)() noexcept { return LLONG_MIN; }
  [[nodiscard]] static constexpr long long(max)() noexcept { return LLONG_MAX; }
  [[nodiscard]] static constexpr long long lowest() noexcept { return (min)(); }
  [[nodiscard]] static constexpr long long epsilon() noexcept { return 0; }
  [[nodiscard]] static constexpr long long round_error() noexcept { return 0; }
  [[nodiscard]] static constexpr long long denorm_min() noexcept { return 0; }
  [[nodiscard]] static constexpr long long infinity() noexcept { return 0; }
  [[nodiscard]] static constexpr long long quiet_NaN() noexcept { return 0; }

  [[nodiscard]] static constexpr long long signaling_NaN() noexcept {
    return 0;
  }
};

#ifdef _NATIVE_WCHAR_T_DEFINED
template <>
struct numeric_limits<unsigned short> : public numeric_limits_integral {
  static constexpr bool is_modulo{true};
  static constexpr int digits{16};
  static constexpr int digits10{4};

  [[nodiscard]] static constexpr unsigned short(min)() noexcept { return 0; }

  [[nodiscard]] static constexpr unsigned short(max)() noexcept {
    return USHRT_MAX;
  }
  [[nodiscard]] static constexpr unsigned short lowest() noexcept {
    return (min)();
  }

  [[nodiscard]] static constexpr unsigned short epsilon() noexcept { return 0; }

  [[nodiscard]] static constexpr unsigned short round_error() noexcept {
    return 0;
  }
  [[nodiscard]] static constexpr unsigned short denorm_min() noexcept {
    return 0;
  }

  [[nodiscard]] static constexpr unsigned short infinity() noexcept {
    return 0;
  }
  [[nodiscard]] static constexpr unsigned short quiet_NaN() noexcept {
    return 0;
  }
  [[nodiscard]] static constexpr unsigned short signaling_NaN() noexcept {
    return 0;
  }
};
#endif  // _NATIVE_WCHAR_T_DEFINED

template <>
struct numeric_limits<unsigned int> : public numeric_limits_integral {
  static constexpr bool is_modulo{true};
  static constexpr int digits{32};
  static constexpr int digits10{9};

  [[nodiscard]] static constexpr unsigned int(min)() noexcept { return 0; }

  [[nodiscard]] static constexpr unsigned int(max)() noexcept {
    return UINT_MAX;
  }
  [[nodiscard]] static constexpr unsigned int lowest() noexcept {
    return (min)();
  }

  [[nodiscard]] static constexpr unsigned int epsilon() noexcept { return 0; }

  [[nodiscard]] static constexpr unsigned int round_error() noexcept {
    return 0;
  }
  [[nodiscard]] static constexpr unsigned int denorm_min() noexcept {
    return 0;
  }

  [[nodiscard]] static constexpr unsigned int infinity() noexcept { return 0; }
  [[nodiscard]] static constexpr unsigned int quiet_NaN() noexcept { return 0; }

  [[nodiscard]] static constexpr unsigned int signaling_NaN() noexcept {
    return 0;
  }
};

template <>
struct numeric_limits<unsigned long> : public numeric_limits_integral {
  static_assert(sizeof(unsigned int) == sizeof(unsigned long),
                "LLP64 assumption");
  static constexpr bool is_modulo{true};
  static constexpr int digits{32};
  static constexpr int digits10{9};

  [[nodiscard]] static constexpr unsigned long(min)() noexcept { return 0; }

  [[nodiscard]] static constexpr unsigned long(max)() noexcept {
    return ULONG_MAX;
  }
  [[nodiscard]] static constexpr unsigned long lowest() noexcept {
    return (min)();
  }

  [[nodiscard]] static constexpr unsigned long epsilon() noexcept { return 0; }

  [[nodiscard]] static constexpr unsigned long round_error() noexcept {
    return 0;
  }
  [[nodiscard]] static constexpr unsigned long denorm_min() noexcept {
    return 0;
  }

  [[nodiscard]] static constexpr unsigned long infinity() noexcept { return 0; }

  [[nodiscard]] static constexpr unsigned long quiet_NaN() noexcept {
    return 0;
  }
  [[nodiscard]] static constexpr unsigned long signaling_NaN() noexcept {
    return 0;
  }
};

template <>
struct numeric_limits<unsigned long long> : public numeric_limits_integral {
  static constexpr bool is_modulo{true};
  static constexpr int digits{64};
  static constexpr int digits10{19};

  [[nodiscard]] static constexpr unsigned long long(min)() noexcept {
    return 0;
  }
  [[nodiscard]] static constexpr unsigned long long(max)() noexcept {
    return ULLONG_MAX;
  }
  [[nodiscard]] static constexpr unsigned long long lowest() noexcept {
    return (min)();
  }
  [[nodiscard]] static constexpr unsigned long long epsilon() noexcept {
    return 0;
  }
  [[nodiscard]] static constexpr unsigned long long round_error() noexcept {
    return 0;
  }
  [[nodiscard]] static constexpr unsigned long long denorm_min() noexcept {
    return 0;
  }
  [[nodiscard]] static constexpr unsigned long long infinity() noexcept {
    return 0;
  }
  [[nodiscard]] static constexpr unsigned long long quiet_NaN() noexcept {
    return 0;
  }
  [[nodiscard]] static constexpr unsigned long long signaling_NaN() noexcept {
    return 0;
  }
};

struct numeric_limits_floating_point
    : numeric_limit_base {  // base for floating-point types
  static constexpr float_denorm_style has_denorm{denorm_present};
  static constexpr bool has_infinity{true};
  static constexpr bool has_quiet_NaN{true};
  static constexpr bool has_signaling_NaN{true};
  static constexpr bool is_bounded{true};
  static constexpr bool is_iec559{true};
  static constexpr bool is_signed{true};
  static constexpr bool is_specialized{true};
  static constexpr float_round_style round_style{round_to_nearest};
  static constexpr int radix{FLT_RADIX};
};

template <>
struct numeric_limits<float> : public numeric_limits_floating_point {
  static constexpr int digits = FLT_MANT_DIG;
  static constexpr int digits10 = FLT_DIG;
  static constexpr int max_digits10 = 9;
  static constexpr int max_exponent = FLT_MAX_EXP;
  static constexpr int max_exponent10 = FLT_MAX_10_EXP;
  static constexpr int min_exponent = FLT_MIN_EXP;
  static constexpr int min_exponent10 = FLT_MIN_10_EXP;

  [[nodiscard]] static constexpr float(min)() noexcept { return FLT_MIN; }

  [[nodiscard]] static constexpr float(max)() noexcept { return FLT_MAX; }

  [[nodiscard]] static constexpr float lowest() noexcept { return -(max)(); }

  [[nodiscard]] static constexpr float epsilon() noexcept {
    return FLT_EPSILON;
  }

  [[nodiscard]] static constexpr float round_error() noexcept { return 0.5F; }

  [[nodiscard]] static constexpr float denorm_min() noexcept {
    return FLT_TRUE_MIN;
  }

  [[nodiscard]] static constexpr float infinity() noexcept {
    return __builtin_huge_valf();
  }

  [[nodiscard]] static constexpr float quiet_NaN() noexcept {
    return __builtin_nanf("0");
  }

  [[nodiscard]] static constexpr float signaling_NaN() noexcept {
    return __builtin_nansf("1");
  }
};

template <>
struct numeric_limits<double> : public numeric_limits_floating_point {
  static constexpr int digits = DBL_MANT_DIG;
  static constexpr int digits10 = DBL_DIG;
  static constexpr int max_digits10 = 17;
  static constexpr int max_exponent = DBL_MAX_EXP;
  static constexpr int max_exponent10 = DBL_MAX_10_EXP;
  static constexpr int min_exponent = DBL_MIN_EXP;
  static constexpr int min_exponent10 = DBL_MIN_10_EXP;

  [[nodiscard]] static constexpr double(min)() noexcept { return DBL_MIN; }

  [[nodiscard]] static constexpr double(max)() noexcept { return DBL_MAX; }

  [[nodiscard]] static constexpr double lowest() noexcept { return -(max)(); }

  [[nodiscard]] static constexpr double epsilon() noexcept {
    return DBL_EPSILON;
  }

  [[nodiscard]] static constexpr double round_error() noexcept { return 0.5; }

  [[nodiscard]] static constexpr double denorm_min() noexcept {
    return DBL_TRUE_MIN;
  }

  [[nodiscard]] static constexpr double infinity() noexcept {
    return __builtin_huge_val();
  }

  [[nodiscard]] static constexpr double quiet_NaN() noexcept {
    return __builtin_nan("0");
  }

  [[nodiscard]] static constexpr double signaling_NaN() noexcept {
    return __builtin_nans("1");
  }
};

template <>
struct numeric_limits<long double> : public numeric_limits_floating_point {
  static constexpr int digits = LDBL_MANT_DIG;
  static constexpr int digits10 = LDBL_DIG;
  static constexpr int max_digits10 = 17;
  static constexpr int max_exponent = LDBL_MAX_EXP;
  static constexpr int max_exponent10 = LDBL_MAX_10_EXP;
  static constexpr int min_exponent = LDBL_MIN_EXP;
  static constexpr int min_exponent10 = LDBL_MIN_10_EXP;

  [[nodiscard]] static constexpr long double(min)() noexcept {
    return LDBL_MIN;
  }

  [[nodiscard]] static constexpr long double(max)() noexcept {
    return LDBL_MAX;
  }

  [[nodiscard]] static constexpr long double lowest() noexcept {
    return -(max)();
  }

  [[nodiscard]] static constexpr long double epsilon() noexcept {
    return LDBL_EPSILON;
  }

  [[nodiscard]] static constexpr long double round_error() noexcept {
    return 0.5L;
  }

  [[nodiscard]] static constexpr long double denorm_min() noexcept {
    return LDBL_TRUE_MIN;
  }

  [[nodiscard]] static constexpr long double infinity() noexcept {
    return __builtin_huge_val();
  }

  [[nodiscard]] static constexpr long double quiet_NaN() noexcept {
    return __builtin_nan("0");
  }

  [[nodiscard]] static constexpr long double signaling_NaN() noexcept {
    return __builtin_nans("1");
  }
};

}  // namespace ktl
#endif