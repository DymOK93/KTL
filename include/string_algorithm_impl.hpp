#pragma once
#include <algorithm.hpp>

namespace ktl {
namespace str::details {
template <class Traits, class CharT, class SizeType>
constexpr SizeType find_ch(const CharT* str,
                           CharT ch,
                           SizeType length,
                           SizeType start_pos,
                           SizeType npos) {
  if (start_pos >= length) {
    return npos;
  }
  const CharT* found_ptr{Traits::find(str + start_pos, length - start_pos, ch)};
  return found_ptr ? static_cast<SizeType>(found_ptr - str) : npos;
}

template <class Traits, class CharT, class SizeType>
constexpr SizeType rfind_ch(const CharT* str,
                            CharT ch,
                            SizeType length,
                            SizeType start_pos,
                            SizeType npos) {
  if (start_pos >= length) {
    return npos;
  }
  for (SizeType idx = length; idx > 0; --idx) {
    const SizeType pos{idx - 1};
    if (Traits::eq(str[pos], ch)) {
      return pos;
    }
  }
  return npos;
}

template <class Traits, class CharT, class SizeType>
constexpr SizeType find_substr(const CharT* str,
                               SizeType str_start_pos,
                               SizeType str_length,
                               const CharT* substr,
                               SizeType substr_length,
                               SizeType npos) {
  if (substr_length > str_length) {
    return npos;
  }
  const CharT* str_end{str + str_length};
  const auto found_ptr{find_subrange(
      str + str_start_pos, str_end, substr, substr + substr_length,
      [](CharT lhs, CharT rhs) { return Traits::eq(lhs, rhs); })};
  return found_ptr != str_end ? static_cast<SizeType>(found_ptr - str) : npos;
}
}  // namespace str::details
}  // namespace ktl