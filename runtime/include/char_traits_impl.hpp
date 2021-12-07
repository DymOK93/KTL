#pragma once
#include <limits_impl.hpp>
#include <type_traits_impl.hpp>

#include <ntddk.h>
#include <stdio.h>  // Defines the EOF

namespace ktl {
template <typename CharT, typename IntT>
struct char_traits_base {
  using char_type = CharT;
  using int_type = IntT;

  static char_type* assign(char_type* dst,
                           size_t count,
                           const char_type& ch) noexcept {
    while (count--) {
      *dst++ = ch;
    }
    return dst;
  }

  static constexpr void assign(char_type& dst, const char_type& src) noexcept {
    dst = src;
  }

  [[nodiscard]] static constexpr bool eq(const char_type& lhs,
                                         const char_type& rhs) noexcept {
    return lhs == rhs;
  }

  [[nodiscard]] static constexpr bool lt(const char_type& lhs,
                                         const char_type& rhs) noexcept {
    return lhs < rhs;
  }

  static char_type* move(char_type* dst,
                         const char_type* src,
                         size_t count) noexcept {
    return static_cast<char_type*>(
        memmove(dst, src, count * sizeof(char_type)));
  }

  static char_type* copy(char_type* dst,
                         const char_type* src,
                         size_t count) noexcept {
    return static_cast<char_type*>(memcpy(dst, src, count * sizeof(char_type)));
  }

  [[nodiscard]] static constexpr int compare(const char_type* str1,
                                             const char_type* str2,
                                             size_t count) noexcept {
    for (; count > 0; --count, ++str1, ++str2) {
      if (*str1 != *str2) {
        return *str1 < *str2 ? -1 : 1;
      }
    }
    return 0;
  }

  static constexpr size_t length(const char_type* str) noexcept {
    size_t count{0};
    while (*str != char_type{}) {
      ++str;
      ++count;
    }
    return count;
  }

  [[nodiscard]] static constexpr const char_type*
  find(const char_type* str, size_t count, const char_type& ch) noexcept {
    for (; count > 0; --count, ++str) {
      if (*str == ch) {
        return str;
      }
    }
    return nullptr;
  }

  [[nodiscard]] static constexpr char_type to_char_type(
      const int_type& meta) noexcept {
    return static_cast<char_type>(meta);
  }

  [[nodiscard]] static constexpr int_type to_int_type(
      const char_type& ch) noexcept {
    return static_cast<int_type>(ch);
  }

  [[nodiscard]] static constexpr bool eq_int_type(
      const int_type& lhs,
      const int_type& rhs) noexcept {
    return lhs == rhs;
  }

  [[nodiscard]] static constexpr int_type eof() noexcept {
    return static_cast<int_type>(EOF);
  }

  [[nodiscard]] static constexpr int_type not_eof(
      const int_type& meta) noexcept {
    return meta != eof() ? meta : !eof();
  }
};

namespace str::details {
template <typename Elem>
struct narrow_char_traits
    : char_traits_base<Elem, int> {  // 1-byte types: char, char8_t, etc.
  using MyBase = char_traits_base<Elem, int>;
  using char_type = Elem;
  using int_type = int;

  static char_type* assign(char_type* ptr,
                           size_t count,
                           char_type ch) noexcept {
    return static_cast<char_type*>(memset(ptr, ch, count));
  }

  static constexpr void assign(char_type& dst, char_type src) noexcept {
    dst = src;
  }

  static constexpr bool eq(char_type lhs, char_type rhs) noexcept {
    return lhs == rhs;
  }

  static char_type* move(char_type* dst,
                         const char_type* src,
                         size_t count) noexcept {
    return MyBase::move(dst, src, count);
  }

  static char_type* copy(char_type* dst,
                         const char_type* src,
                         size_t count) noexcept {
    return MyBase::copy(dst, src, count);
  }

  static constexpr int compare(const char_type* str1,
                               const char_type* str2,
                               size_t count) noexcept {
    // char8_t is also supported
    return __builtin_memcmp(str1, str2, count);
  }

  static constexpr size_t length(const char_type* str) noexcept {
    // This check is required for char8_t
    if constexpr (is_same_v<char_type, char>) {
      return __builtin_strlen(str);
    } else {
      return MyBase::length(str);  // Not an strlen because it isn't constexpr
    }
  }

  [[nodiscard]] static constexpr const char_type* find(const char_type* str,
                                                       size_t count,
                                                       char_type ch) noexcept {
    // This check is required for char8_t
    if constexpr (is_same_v<char_type, char>) {
      return __builtin_char_memchr(str, ch, count);
    } else {
      // Not an memchr because it isn't constexpr
      return MyBase::find(str, ch, count);
    }
  }

  [[nodiscard]] static constexpr char_type to_char_type(
      int_type meta) noexcept {
    return static_cast<char_type>(meta);
  }

  [[nodiscard]] static constexpr int_type to_int_type(char_type ch) noexcept {
    return static_cast<int_type>(ch);
  }

  [[nodiscard]] static constexpr bool eq_int_type(int_type lhs,
                                                  int_type rhs) noexcept {
    return lhs == rhs;
  }

  [[nodiscard]] static constexpr int_type eof() noexcept { return EOF; }

  [[nodiscard]] static constexpr int_type not_eof(int_type meta) noexcept {
    return meta != eof() ? meta : !eof();
  }
};

template <typename Elem>
struct wide_char_traits
    : char_traits_base<Elem, wint_t> {  // 2-byte types: wchar_t,
                                        // char16_t, etc.
  using MyBase = char_traits_base<Elem, wint_t>;
  using char_type = Elem;
  using int_type = wint_t;

  static char_type* assign(char_type* ptr,
                           size_t count,
                           char_type ch) noexcept {
    if (ch == static_cast<char_type>(0)) {
      memset(ptr, 0, count * sizeof(char_type));
    } else {
      /*
       * memset is almost always implemented as compiler intrinsic,
       * but wmemset in MSVC implemented as a simple loop
       * and can be optimized later into multiple movs or rep stosw
       */
      wmemset(ptr, ch, count);
    }
    return ptr;
  }

  static constexpr void assign(char_type& dst, char_type src) noexcept {
    dst = src;
  }

  static constexpr bool eq(char_type lhs, char_type rhs) noexcept {
    return lhs == rhs;
  }

  static char_type* move(char_type* dst,
                         const char_type* src,
                         size_t count) noexcept {
    return MyBase::move(dst, src, count);
  }

  static char_type* copy(char_type* dst,
                         const char_type* src,
                         size_t count) noexcept {
    return MyBase::copy(dst, src, count);
  }

  static constexpr int compare(const char_type* str1,
                               const char_type* str2,
                               size_t count) noexcept {
    if constexpr (is_same_v<char_type, wchar_t>) {
      return __builtin_wmemcmp(str1, str2, count);
    } else {
      // Not an wmemcmp because it isn't constexpr
      return MyBase::compare(str1, str2, count);
    }
  }

  static constexpr size_t length(const char_type* str) noexcept {
    if constexpr (is_same_v<char_type, wchar_t>) {
      return __builtin_wcslen(str);
    } else {
      // Not an wcslen because it isn't constexpr
      return MyBase::length(str);
    }
  }

  [[nodiscard]] static constexpr const char_type* find(const char_type* str,
                                                       size_t count,
                                                       char_type ch) noexcept {
    if constexpr (is_same_v<char_type, wchar_t>) {
      return __builtin_wmemchr(str, ch, count);
    } else {
      // Not an wmemchr because it isn't constexpr
      return MyBase::find(str, ch, count);
    }
  }

  [[nodiscard]] static constexpr char_type to_char_type(
      int_type meta) noexcept {
    return static_cast<char_type>(meta);
  }

  [[nodiscard]] static constexpr int_type to_int_type(char_type ch) noexcept {
    return static_cast<int_type>(ch);
  }

  [[nodiscard]] static constexpr bool eq_int_type(int_type lhs,
                                                  int_type rhs) noexcept {
    return lhs == rhs;
  }

  [[nodiscard]] static constexpr int_type eof() noexcept {
    return WEOF;  // Already cast to the desired type
  }

  [[nodiscard]] static constexpr int_type not_eof(int_type meta) noexcept {
    return meta != eof() ? meta : !eof();
  }
};
}  // namespace str::details

template <typename CharT>
struct char_traits;

template <>
struct char_traits<char> : str::details::narrow_char_traits<char> {};
// template <>
// struct char_traits<char8_t> : str::details::narrow_char_traits<char8_t> {};

template <>
struct char_traits<wchar_t> : str::details::wide_char_traits<wchar_t> {};
template <>
struct char_traits<char16_t> : str::details::wide_char_traits<char16_t> {};
}  // namespace ktl