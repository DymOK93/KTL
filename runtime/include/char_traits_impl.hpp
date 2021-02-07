#pragma once
#include <basic_types.h>
#include <ntddk.h>
#include <wchar.h>
#include <type_traits_impl.hpp>

#define EOF -1

namespace ktl {
template <typename CharT, typename IntT>
struct char_traits_base {
  using char_type = CharT;
  using int_type = IntT;

  static char_type* assign(char_type* dst,
                           size_t count,
                           const char_type ch) noexcept {
    for (char_type* place = dst; count > 0; --count, ++place) {
      *place = ch;
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

  static char_type* move(char_type* const dst,
                         const char_type* src,
                         size_t count) noexcept {
    return static_cast<char_type*>(
        memmove(dst, src, count * sizeof(char_type)));
  }

  static char_type* copy(char_type* const dst,
                         const char_type* src,
                         size_t count) noexcept {
    return static_cast<char_type*>(memcpy(dst, src, count * sizeof(char_type)));
  }

  [[nodiscard]] static constexpr int compare(const char_type* str1,
                                             const char_type* str2,
                                             size_t count) noexcept {
    for (; count > 0; --count, ++str1, ++str2) {
      if (*str1 != *str2) {
        return *str < *str2 ? -1 : 1;
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
struct narrow_char_traits {  // 1-byte types: char, char8_t, etc.
 public:
  using char_type = Elem;
  using int_type = int;

  static char_type* assign(char_type* ptr,
                           size_t count,
                           char_type ch) noexcept {
    return reinterpret_cast<char_type*>(memset(ptr, ch, count));
  }

  static constexpr void assign(char_type& dst, const char_type& src) noexcept {
    dst = src;
  }

  static constexpr bool eq(char_type lhs, char_type rhs) noexcept {
    return lhs == rhs;
  }

  static char_type* move(char_type* dst,
                         const char_type* src,
                         size_t count) noexcept {
    return reinterpret_cast<char_type*>(memmove(dst, src, count));
  }

  static constexpr char_type* copy(char_type* dst,
                                   const char_type* src,
                                   size_t count) noexcept {
    return reinterpret_cast<char_type*>(memcpy(dst, src, count));
  }

  static constexpr int compare(const char_type* str1,
                               const char_type* str2,
                               size_t count) {
    if constexpr (is_same_v<char_type, wchar_t>)
      noexcept { return __builtin_memcmp(str1, str2, count); }
    else {
      return char_traits_base<char_type, int_type>::compare(str1, str2, count);
    }
  }

  static constexpr size_t length(const char_type* str) noexcept {
    if constexpr (is_same_v<char_type, char>) {
      return __builtin_strlen(str);
    } else {
      return char_traits_base<char_type, int_type>::length(str);
    }
  }

  [[nodiscard]] static constexpr const char_type*
  find(const char_type* str, size_t count, const char_type& ch) noexcept {
    if constexpr (is_same_v<char_type, wchar_t>) {
      return __builtin_char_memchr(str, ch, count);
    } else {
      return char_traits_base<char_type, int_type>::find(str, count, ch);
    }
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

template <typename Elem>
struct wide_char_traits {  // 2-byte types: wchar_t, char16_t, etc.
 public:
  using char_type = Elem;
  using int_type = unsigned short;

  static char_type* assign(char_type* ptr,
                           size_t count,
                           char_type ch) noexcept {
    return reinterpret_cast<char_type*>(wmemset(ptr, ch, count));
  }

  static constexpr void assign(char_type& dst, const char_type& src) noexcept {
    dst = src;
  }

  static constexpr bool eq(char_type lhs, char_type rhs) noexcept {
    return lhs == rhs;
  }

  static char_type* move(char_type* dst,
                         const char_type* src,
                         size_t count) noexcept {
    return reinterpret_cast<char_type*>(wmemmove(dst, src, count));
  }

  static constexpr char_type* copy(char_type* dst,
                                   const char_type* src,
                                   size_t count) noexcept {
    return reinterpret_cast<Elem*>(wmemcpy(dst, src, count));
  }

  static constexpr int compare(const char_type* str1,
                               const char_type* str2,
                               size_t count) {
    if constexpr (is_same_v<char_type, wchar_t>)
      noexcept { return __builtin_wmemcmp(str1, str2, count); }
    else {
      return char_traits_base<char_type, int_type>::compare(str1, str2, count);
    }
  }

  static constexpr size_t length(const char_type* str) noexcept {
    if constexpr (is_same_v<char_type, wchar_t>) {
      return __builtin_wcslen(str);
    } else {
      return char_traits_base<char_type, int_type>::length(str);
    }
  }

  [[nodiscard]] static constexpr const char_type*
  find(const char_type* str, size_t count, const char_type& ch) noexcept {
    if constexpr (is_same_v<char_type, wchar_t>) {
      return __builtin_wmemchr(str, ch, count);
    } else {
      return char_traits_base<char_type, int_type>::find(str, count, ch);
    }
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