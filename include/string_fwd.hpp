#pragma once
#include <allocator.hpp>
#include <char_traits.hpp>

namespace ktl {
using native_ansi_str_t = ANSI_STRING;
using native_unicode_str_t = UNICODE_STRING;

namespace str::details {
template <typename NativeStrTy>
class native_string_traits_base {
 public:
  using value_type = remove_pointer_t<decltype(NativeStrTy{}.Buffer)>;
  using size_type = decltype(NativeStrTy{}.Length);

 public:
  static constexpr value_type* get_buffer(NativeStrTy& str) noexcept {
    return str.Buffer;
  }

  static constexpr const value_type* get_buffer(
      const NativeStrTy& str) noexcept {
    return str.Buffer;
  }

  static constexpr void set_buffer(NativeStrTy& str,
                                   value_type* buffer) noexcept {
    str.Buffer = buffer;
  }

  static constexpr size_type get_size(const NativeStrTy& str) noexcept {
    return bytes_count_to_ch(str.Length);
  }

  static constexpr void set_size(NativeStrTy& str,
                                 size_type ch_count) noexcept {
    str.Length = ch_count_to_bytes(ch_count);
  }

  static constexpr void increase_size(NativeStrTy& str,
                                      size_type addendum) noexcept {
    str.Length += ch_count_to_bytes(addendum);
  }

  static constexpr void decrease_size(NativeStrTy& str,
                                      size_type subtrahend) noexcept {
    str.Length -= ch_count_to_bytes(subtrahend);
  }

  static constexpr size_type get_capacity(const NativeStrTy& str) noexcept {
    return bytes_count_to_ch(str.MaximumLength);
  }

  static constexpr void set_capacity(NativeStrTy& str,
                                     size_type new_capacity) noexcept {
    str.MaximumLength = ch_count_to_bytes(new_capacity);
  }

  static constexpr size_type get_max_size() noexcept {
    return bytes_count_to_ch((numeric_limits<size_type>::max)() - 1);
  }

 private:
  static constexpr size_type bytes_count_to_ch(size_type bytes_count) noexcept {
    return bytes_count / sizeof(value_type);
  }

  static constexpr size_type ch_count_to_bytes(size_type ch_count) noexcept {
    return ch_count * sizeof(value_type);
  }
};

template <typename NativeStrTy>
struct native_string_traits;

template <>
struct native_string_traits<native_unicode_str_t>
    : public native_string_traits_base<native_unicode_str_t> {};

template <>
struct native_string_traits<native_ansi_str_t>
    : public native_string_traits_base<native_ansi_str_t> {};

template <typename NativeStrTy, template <typename... CharT> class Traits>
class basic_winnt_string_view;

template <size_t SsoBufferChCount,
          typename NativeStrTy,
          template <typename... CharT>
          class Traits,
          template <typename... CharT>
          class Alloc>
class basic_winnt_string;

inline constexpr size_t DEFAULT_SSO_CH_BUFFER_COUNT{16};
}  // namespace str::details

template <typename CharT>
using basic_winnt_string_view =
    str::details::basic_winnt_string_view<native_ansi_str_t, char_traits>;

template <template <typename... CharT> class Traits>
using basic_ansi_string_view =
    str::details::basic_winnt_string_view<native_ansi_str_t, Traits>;

using ansi_string_view =
    str::details::basic_winnt_string_view<native_ansi_str_t, char_traits>;

template <template <typename... CharT> class Traits>
using basic_unicode_string_view =
    str::details::basic_winnt_string_view<native_unicode_str_t, Traits>;

using unicode_string_view =
    str::details::basic_winnt_string_view<native_unicode_str_t, char_traits>;

template <size_t SsoBufferChCount,
          template <typename... CharT> class Traits = char_traits,
          template <typename... CharT> class Alloc = basic_paged_allocator>
using basic_ansi_string = str::details::
    basic_winnt_string<SsoBufferChCount, native_ansi_str_t, Traits, Alloc>;

using ansi_string =
    basic_ansi_string<str::details::DEFAULT_SSO_CH_BUFFER_COUNT>;
using ansi_string_non_paged =
    basic_ansi_string<str::details::DEFAULT_SSO_CH_BUFFER_COUNT,
                      char_traits,
                      basic_non_paged_allocator>;

template <size_t SsoBufferChCount,
          template <typename... CharT> class Traits = char_traits,
          template <typename... CharT> class Alloc = basic_paged_allocator>
using basic_unicode_string = str::details::
    basic_winnt_string<SsoBufferChCount, native_unicode_str_t, Traits, Alloc>;

using unicode_string =
    basic_unicode_string<str::details::DEFAULT_SSO_CH_BUFFER_COUNT>;
using unicode_string_non_paged =
    basic_unicode_string<str::details::DEFAULT_SSO_CH_BUFFER_COUNT,
                         char_traits,
                         basic_non_paged_allocator>;

inline namespace literals {
inline namespace string_literals {
ansi_string operator""_as(const char* str, size_t length);
ansi_string_non_paged operator""_asnp(const char* str, size_t length);

unicode_string operator""_us(const wchar_t* str, size_t length);
unicode_string_non_paged operator""_usnp(const wchar_t* str, size_t length);
}  // namespace string_literals
}  // namespace literals
}  // namespace ktl