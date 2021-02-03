#pragma once
#include <allocator.hpp>
#include <char_traits.hpp>

namespace ktl {
template <size_t SsoBufferChCount,
          template <typename... CharT>
          class Traits,
          template <typename... CharT>
          class Alloc>
class basic_unicode_string;

using unicode_string =
    basic_unicode_string<16, char_traits, basic_paged_allocator>;
using unicode_string_non_paged =
    basic_unicode_string<16, char_traits, basic_non_paged_allocator>;

using native_unicode_str_t = UNICODE_STRING;
}  // namespace ktl