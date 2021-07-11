#pragma once
#include <limits.h>

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
  static constexpr bool has_denorm_loss = false;
  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool is_bounded = false;
  static constexpr bool is_exact = false;
  static constexpr bool is_iec559 = false;
  static constexpr bool is_integer = false;
  static constexpr bool is_modulo = false;
  static constexpr bool is_signed = false;
  static constexpr bool is_specialized = false;
  static constexpr bool tinyness_before = false;
  static constexpr bool traps = false;
  static constexpr float_round_style round_style = round_toward_zero;
  static constexpr int digits = 0;
  static constexpr int digits10 = 0;
  static constexpr int max_digits10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;
  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int radix = 0;
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
  static constexpr int digits = 1;

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
  static constexpr bool is_signed = (CHAR_MIN != 0);
  static constexpr bool is_modulo = (CHAR_MIN == 0);
  static constexpr int digits = (8 - (CHAR_MIN != 0));
  static constexpr int digits10 = 2;

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
  static constexpr bool is_signed = true;
  static constexpr int digits = 7;
  static constexpr int digits10 = 2;

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
  static constexpr bool is_modulo = true;
  static constexpr int digits = 8;
  static constexpr int digits10 = 2;

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
  static constexpr bool is_modulo = true;
  static constexpr int digits = 16;
  static constexpr int digits10 = 4;

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
  static constexpr bool is_modulo = true;
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
  static constexpr bool is_modulo = true;
  static constexpr int digits = 16;
  static constexpr int digits10 = 4;

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
  static constexpr bool is_signed = true;
  static constexpr int digits = 15;
  static constexpr int digits10 = 4;

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
  static constexpr bool is_signed = true;
  static constexpr int digits = 31;
  static constexpr int digits10 = 9;

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
  static constexpr bool is_signed = true;
  static constexpr int digits = 31;
  static constexpr int digits10 = 9;

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
  static constexpr bool is_signed = true;
  static constexpr int digits = 63;
  static constexpr int digits10 = 18;

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
  static constexpr bool is_modulo = true;
  static constexpr int digits = 16;
  static constexpr int digits10 = 4;

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
  static constexpr bool is_modulo = true;
  static constexpr int digits = 32;
  static constexpr int digits10 = 9;

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
  static constexpr bool is_modulo = true;
  static constexpr int digits = 32;
  static constexpr int digits10 = 9;

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
  static constexpr bool is_modulo = true;
  static constexpr int digits = 64;
  static constexpr int digits10 = 19;

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
}  // namespace ktl