#pragma once
#include <type_traits.hpp>
#include <utility.hpp>

// TODO: conditional noexcept for operator[]

namespace ktl {
namespace str::details {
template <class CharT>
constexpr size_t null_terminated_string_length(const CharT* ptr) {
  size_t length{0};
  while (ptr[length] != static_cast<CharT>(0)) {
    ++length;
  }
  return length;
}

template <class StringOrChar, class = void>
struct internal_size_type_impl {
  using type = size_t;
};

template <class String>
struct internal_size_type_impl<String, void_t<typename String::size_type>> {
  using type = typename String::size_type;
};

template <class StringOrChar>
using internal_size_type = typename internal_size_type_impl<StringOrChar>::type;

template <class String, class = void>
struct get_npos {
  using size_type = internal_size_type<String>;
  static constexpr size_type value{
      static_cast<size_type>(-1)};  // До написания собственного limits
};

template <class String>
struct get_npos<String, void_t<decltype(String::npos)>> {
  static constexpr internal_size_type<String> value{String::npos};
};

template <class String>
inline constexpr internal_size_type<String> get_npos_v =
    get_npos<String>::value;

template <class StrTy, typename CharT>
constexpr internal_size_type<StrTy> find_character_impl(
    const StrTy& str,
    internal_size_type<StrTy> str_pos,
    internal_size_type<StrTy> str_len,
    CharT ch) noexcept {
  auto segment_length{str_len + str_pos};
  for (; str_pos < segment_length && str[str_pos] != ch; ++str_pos)
    ;
  return str_pos < str_len ? str_pos : get_npos_v<StrTy>;
}

template <class StrTy, class SubstrTy>
constexpr internal_size_type<StrTy> find_substr_impl(
    const StrTy& str,
    internal_size_type<StrTy> str_pos,
    internal_size_type<StrTy> count,
    const SubstrTy& substr,
    internal_size_type<SubstrTy> substr_len) noexcept {
  using str_size_type = internal_size_type<StrTy>;
  using substr_size_type = internal_size_type<StrTy>;
  auto max_pos{static_cast<str_size_type>(str_pos + count)};
  for (; str_pos < max_pos;) {
    auto substr_match_end{static_cast<str_size_type>(0)};
    for (; substr_match_end < substr_len &&
           str[substr_match_end + str_pos] ==
               substr[static_cast<substr_size_type>(substr_match_end)];
         ++substr_match_end)
      ;
    if (substr_match_end < substr_len) {
      str_pos += substr_match_end + 1;
    } else {
      break;
    }
  }
  return str_pos < max_pos ? str_pos : get_npos_v<StrTy>;
}
}  // namespace str::details

template <class StringOrChar>
constexpr str::details::internal_size_type<StringOrChar> length(
    const StringOrChar& val) {
  if constexpr (is_array_v<StringOrChar>) {  //Для C-style массивов
                                             // size() вернёт размер
                                             //без учёта нуль-символа
    return size(val) > 0 ? size(val) - 1 : 0;
  } else if constexpr (is_pointer_v<StringOrChar>) {
    return str::details::null_terminated_string_length(val);
  } else if constexpr (!is_trivial_v<StringOrChar>) {
    return size(val);
  } else {
    return sizeof(val);  //Для char
  }
}

template <class... Strings>
constexpr size_t summary_length(const Strings&... strs) {
  size_t summary_length{0};
  ((summary_length += static_cast<size_t>(length(strs))), ...);
  return summary_length;
}

template <class StrTy, class SubstrTy>
constexpr str::details::internal_size_type<StrTy> find_substr(
    const StrTy& str,
    str::details::internal_size_type<StrTy> str_pos,
    const SubstrTy& substr) noexcept {
  auto str_length{
      str::details::internal_size_type<StrTy>(length(str) - str_pos)};
  return str::details::find_substr_impl(str, str_pos, str_length, substr,
                                        length(substr));
}

template <class StrTy, class SubstrTy>
constexpr size_t find_substr(const StrTy& str,
                             const SubstrTy& substr) noexcept {
  return find_substr(
      str, static_cast<str::details::internal_size_type<StrTy>>(0), substr);
}

template <class StrTy, typename CharT>
constexpr str::details::internal_size_type<StrTy> find_character(
    const StrTy& str,
    str::details::internal_size_type<StrTy> str_pos,
    CharT ch) noexcept {
  return str::details::find_character_impl(str, str_pos, length(str) - str_pos,
                                           ch);
}

template <class StrTy, typename CharT>
constexpr str::details::internal_size_type<StrTy> find_character(
    const StrTy& str,
    CharT ch) noexcept {
  return find_character(str, str::details::internal_size_type<StrTy>(0), ch);
}

namespace str::details {
template <class Container, class = void>
struct has_reserve : public false_type {};

template <class Container>
struct has_reserve<
    Container,
    void_t<decltype(declval<Container>().reserve(declval<size_t>()))>>
    : public true_type {};

template <class Container>
inline constexpr bool has_reserve_v = has_reserve<Container>::value;

template <class Container, class = void>
struct has_remove_prefix : public false_type {};

template <class Container>
struct has_remove_prefix<
    Container,
    void_t<decltype(declval<Container>().remove_prefix(declval<size_t>()))>>
    : public true_type {};

template <class Container>
inline constexpr bool has_remove_prefix_v = has_remove_prefix<Container>::value;

template <class String>
constexpr String make_empty_string(size_t recommended_capacity) {
  String str;
  if constexpr (has_reserve_v<String>) {
    str.reserve(static_cast < internal_size_type<String>(recommended_capacity));
  }
  return str;
}
}  // namespace str::details

template <class OutputStrTy, class StrTy, class SubstrTy1, class SubstrTy2>
constexpr enable_if_t<str::details::has_remove_prefix_v<StrTy>,
                      OutputStrTy>
replace_substr(StrTy str,  // string_view or string_ref
               const SubstrTy1& old_substr,
               const SubstrTy2& new_substr) {
  auto output_str{str::details::make_empty_string<OutputStrTy>(length(str))};
  const auto substr_len{length(old_substr)};

  while (true) {
    auto substr_pos{str::details::find_substr_impl(str, 0, length(str),
                                                   old_substr, substr_len)};
    if (substr_pos == str::details::get_npos_v<StrTy>) {
      output_str += str.substr(0, length(str));
      break;
    }
    output_str.append(str.substr(0, substr_pos));
    output_str.append(new_substr);
    str.remove_prefix(substr_pos + length(old_substr));
  }
  return output_str;
}

template <class OutputStrTy, class StrTy, class SubstrTy1, class SubstrTy2>
constexpr enable_if_t<!str::details::has_remove_prefix_v<StrTy>, OutputStrTy>
replace_substr(const StrTy& str,
               const SubstrTy1& old_substr,
               const SubstrTy2& new_substr) {
  auto output_str{str::details::make_empty_string<OutputStrTy>(length(str))};
  const auto* data{data(str)};
  const auto str_len{length(str)};
  const auto substr_len{length(old_substr)};

  for (size_t start_pos = 0; start_pos < length(str);) {
    auto substr_pos{str::details::find_substr_impl(str, start_pos, str_len,
                                                   old_substr, substr_len)};
    if (substr_pos == str::details::get_npos_v<StrTy>) {
      output_str.append(data + start_pos, length(str) - start_pos) break;
    }
    output_str.append(data + start_pos, substr_pos - start_pos);
    output_str.append(new_substr);
    start_pos += length(substr_pos - start_pos) + length(old_substr);
  }
  return output_str;
}

template <class OutputString, class... Types>
OutputString concatenate(const Types&... strs_and_symbols) {
  OutputString result;
  if constexpr (str::details::has_reserve_v<OutputString>) {
    result.reserve(static_cast<str::details::internal_size_type<OutputString>>(
        summary_length(strs_and_symbols...)));
  }
  ((result.append(strs_and_symbols)), ...);
  return result;
}

template <class StringView, class Predicate>
StringView strict(StringView str, Predicate pred) {
  size_t prefix_length{0};
  for (; prefix_length < str.length() && pred(str[prefix_length]);
       ++prefix_length)
    ;
  str.remove_prefix(prefix_length);

  size_t suffix_length{str.length()};
  for (; suffix_length > 0 && pred(str[suffix_length - 1]); --prefix_length)
    ;

  str.remove_suffix(str.length() - suffix_length);

  return str;
}

template <typename CharT>
constexpr int compare(CharT* lhs,
                      size_t lhs_length,
                      CharT* rhs,
                      size_t rhs_length) {
  if (lhs_length < rhs_length) {
    return -1;
  }
  if (rhs_length < lhs_length) {
    return 1;
  }
  for (size_t idx = 0; idx < lhs_length; ++idx) {
    if (int dif = lhs[idx] - rhs[idx]; dif) {
      return dif;
    }
  }
  return 0;
}

template <typename CharT>
constexpr bool equal(CharT* lhs,
                     size_t lhs_length,
                     CharT* rhs,
                     size_t rhs_length) {
  return compare(lhs, lhs_length, rhs, rhs_length) == 0;
}

}  // namespace ktl
