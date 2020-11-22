#pragma once
#include "string_base.hpp"
#include "string_algorithms.hpp"

#include <ntddk.h>
#include <algorithm>
#include <cstdint>
#include <string_view>
#include <utility>

namespace winapi::kernel {
class unicode_string_view {
 public:
  using value_type = wchar_t;
  using pointer = wchar_t*;
  using const_pointer = const wchar_t*;
  using reference = wchar_t&;
  using const_reference = const wchar_t&;
  using const_iterator = const_pointer;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using size_type = unsigned short;  //Как и в UNICODE_STRING
  using difference_type = int;

  static constexpr size_type npos = (std::numeric_limits<size_type>::max)();

 public:
  constexpr unicode_string_view() noexcept = default;
  constexpr unicode_string_view(const unicode_string_view& other) noexcept =
      default;
  constexpr unicode_string_view& operator=(const unicode_string_view& other) =
      default;

  template <size_t N>
  constexpr unicode_string_view(const wchar_t (&str)[N])
      : m_str{make_unicode_string(str), static_cast<size_type>(N)} {}
  constexpr unicode_string_view(const wchar_t* str)
      : m_str{make_unicode_string(
            str,
            static_cast<size_type>(algo::string::length(str)))} {}
  constexpr unicode_string_view(const wchar_t* str, size_type length)
      : m_str{make_unicode_string(str, length)} {}
  constexpr unicode_string_view(UNICODE_STRING str) : m_str{str} {}
  constexpr unicode_string_view(PCUNICODE_STRING str_ptr) : m_str{*str_ptr} {}

 public:
  constexpr const_iterator begin() const noexcept { return cbegin(); }
  constexpr const_iterator cbegin() const noexcept { return data(); }
  constexpr const_iterator rbegin() const noexcept { return rbegin(); }
  constexpr const_iterator crbegin() const noexcept { return cend(); }
  constexpr const_iterator end() const noexcept { return cend(); }
  constexpr const_iterator cend() const noexcept { return data() + length(); }
  constexpr const_iterator rend() const noexcept { return crend(); }
  constexpr const_iterator crend() const noexcept { return cbegin(); }

 public:
  constexpr const_reference& operator[](size_type idx) const {
    return m_str.Buffer[idx];
  }
  constexpr const_reference& front() const { return m_str.Buffer[0]; }
  constexpr const_reference& back() const { return m_str.Buffer[length() - 1]; }
  constexpr const_pointer data() const { return m_str.Buffer; }

 public:
  constexpr size_type size() const { return m_str.Length; }
  constexpr size_type length() const { return m_str.Length; }
  constexpr bool empty() const { return length() == 0; }

 public:
  constexpr void remove_prefix(size_type shift) { m_str.Buffer += shift; }
  constexpr void remove_suffix(size_type shift) { m_str.Length -= shift; }
  _CONSTEXPR20 void swap(unicode_string_view& other) {
    std::swap(m_str, other.m_str);
  }

 public:
  _CONSTEXPR20 size_type copy(wchar_t* dst,
                              size_type count,
                              size_type pos = 0) {
    auto last{std::copy(begin() + pos,
                        begin() + calc_segment_length(pos, count), dst)};
    return static_cast<size_type>(last - dst);
  }
  _CONSTEXPR20 size_type copy(PUNICODE_STRING dst,
                              size_type count,
                              size_type pos = 0) {
    return copy(dst->Buffer, count, pos);
  }
  constexpr unicode_string_view substr(size_type pos, size_type count = npos) {
    return unicode_string_view(data() + pos, calc_segment_length(pos, count));
  }
  constexpr int compare(unicode_string_view other) {
    return algo::string::compare(data(), size(), std::data(other),
                                 std::size(other));
  }
  constexpr int compare(size_type pos,
                        size_type count,
                        unicode_string_view other) {
    return substr(pos, count).compare(other);
  }
  constexpr int compare(size_type pos,
                        size_type count,
                        unicode_string_view other,
                        size_type other_pos,
                        size_type other_count) {
    return substr(pos, count).compare(other.substr(other_pos, other_count));
  }
  constexpr int compare(const wchar_t* other) {
    return algo::string::compare(data(), size(), other,
                                 algo::string::length(other));
  }
  constexpr int compare(size_type pos, size_type count, const wchar_t* other) {
    return substr(pos, count).compare(other);
  }
  constexpr int compare(size_type pos,
                        size_type count,
                        const wchar_t* other,
                        size_type other_pos,
                        size_type other_count) {
    return substr(pos, count)
        .compare(unicode_string_view(other + other_pos, other_count));
  }
  constexpr bool starts_with(wchar_t ch) { return !empty() && front() == ch; }
  constexpr bool starts_with(const wchar_t* str) {
    size_type str_length{static_cast<size_type>(algo::string::length(str))};
    return compare(0, str_length, str, 0, str_length) == 0;
  }
  constexpr bool starts_with(unicode_string_view str) {
    return compare(0, std::size(str), str) == 0;
  }
  constexpr bool ends_with(wchar_t ch) { return !empty() && back() == ch; }
  constexpr bool ends_with(const wchar_t* str) {
    size_type str_length{static_cast<size_type>(algo::string::length(str))};
    if (str_length > length()) {
      return false;
    }
    auto lhs{substr(length() - str_length, str_length)};
    return lhs.compare(0, std::size(lhs), str, 0, str_length);
  }
  constexpr bool ends_with(unicode_string_view str) {
    size_type str_length{std::size(str)};
    if (str_length > length()) {
      return false;
    }
    auto lhs{substr(length() - str_length, str_length)};
    return lhs.compare(str);
  }
  constexpr size_type find(wchar_t ch, size_type pos = 0) {
    return static_cast<size_type>(
        algo::string::find_character(substr(pos), ch));
  }

  constexpr size_type find(const wchar_t* str, size_type pos = 0) {
    return static_cast<size_type>(algo::string::find_substr(substr(pos), str));
  }

  constexpr size_type find(const wchar_t* str,
                           size_t count,
                           size_type pos = 0) {
    auto lhs{substr(pos)};
    return static_cast<size_type>(algo::string::details::find_substr_impl(
        substr(pos), 0, std::size(lhs), str, count));
  }
  constexpr size_type find(unicode_string_view str, size_type pos = 0) {
    return static_cast<size_type>(algo::string::find_substr(substr(pos), str));
  }

 public:
  PUNICODE_STRING raw_str() { return addressof(m_str); }
  PCUNICODE_STRING raw_str() const { return addressof(m_str); }

#ifndef NO_CXX_STANDARD_LIBRARY
  explicit constexpr operator std::wstring_view() const {
    return std::wstring_view(m_str.Buffer, m_str.Length);
  }
#endif

 private:
  constexpr size_type calc_segment_length(size_type pos, size_type count) {
    return (std::min)(
        length(),
        static_cast<size_type>(
            count +
            pos)); 
  }
  static constexpr UNICODE_STRING make_unicode_string(const value_type* str,
                                                      size_type length) {
    UNICODE_STRING unicode_str{};
    unicode_str.Buffer = dirty::remove_const_from_value(str);
    unicode_str.Length = length;
    unicode_str.MaximumLength = length;
    return unicode_str;
  }

 private:
  UNICODE_STRING m_str{};
};

constexpr bool operator==(unicode_string_view lhs, unicode_string_view rhs) {
  return lhs.compare(rhs) == 0;
}
constexpr bool operator!=(unicode_string_view lhs, unicode_string_view rhs) {
  return !(lhs == rhs);
}
constexpr bool operator<(unicode_string_view lhs, unicode_string_view rhs) {
  return lhs.compare(rhs) < 0;
}
constexpr bool operator>(unicode_string_view lhs, unicode_string_view rhs) {
  return lhs.compare(rhs) > 0;
}
constexpr bool operator<=(unicode_string_view lhs, unicode_string_view rhs) {
  return !(lhs > rhs);
}
constexpr bool operator>=(unicode_string_view lhs, unicode_string_view rhs) {
  return !(lhs < rhs);
}
}  // namespace winapi::kernel

namespace std {
_CONSTEXPR20 void swap(winapi::kernel::unicode_string_view& lhs,
                       winapi::kernel::unicode_string_view& rhs) noexcept {
  return lhs.swap(rhs);
}
}  // namespace std