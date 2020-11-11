#pragma once
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

// TODO: conditional noexcept for operator[]

namespace algo {
namespace string {
namespace details {
template <class CharT>
constexpr size_t null_terminated_string_length(const CharT* ptr) {
  size_t length{0};
  for (; ptr[length] != static_cast<CharT>(0); ++length)
    ;
  return length;
}
template <>
size_t null_terminated_string_length<char>(const char* ptr) {
  return std::strlen(ptr);      //heavily optimized avx-strlen
}

template <class StringOrChar, class = void>
struct internal_size_type_impl {
  using type = size_t;
};

template <class String>
struct internal_size_type_impl<String,
                               std::void_t<typename String::size_type>> {
  using type = typename String::size_type;
};

template <class StringOrChar>
using internal_size_type = typename internal_size_type_impl<StringOrChar>::type;

template <class String, class = void>
struct get_npos {
  using size_type = internal_size_type<String>;
  static constexpr size_type value{
      (std::numeric_limits<size_type>::max)()};  //Скобки требуются во
                                                 //избежание ошибок с макросом
                                                 // max
};

template <class String>
struct get_npos<String, std::void_t<decltype(String::npos)>> {
  static constexpr internal_size_type<String> value{String::npos};
};

template <class String>
inline constexpr details::internal_size_type<String> get_npos_v =
    get_npos<String>::value;

template <class StrTy, typename CharT>
constexpr details::internal_size_type<StrTy> find_character_impl(
    const StrTy& str,
    details::internal_size_type<StrTy> str_pos,
    details::internal_size_type<StrTy> str_len,
    CharT ch) noexcept {
  auto segment_length{str_len + str_pos};
  for (; str_pos < segment_length && str[str_pos] != ch; ++str_pos)
    ;
  return str_pos < str_len ? str_pos : get_npos_v<StrTy>;
}

template <class StrTy, class SubstrTy>
constexpr details::internal_size_type<StrTy> find_substr_impl(
    const StrTy& str,
    details::internal_size_type<StrTy> str_pos,
    details::internal_size_type<StrTy> count,
    const SubstrTy& substr,
    details::internal_size_type<SubstrTy> substr_len) noexcept {
  using str_size_type = details::internal_size_type<StrTy>;
  using substr_size_type = details::internal_size_type<StrTy>;
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
}  // namespace details

template <class StringOrChar>
constexpr details::internal_size_type<StringOrChar> length(
    const StringOrChar& val) {
  if constexpr (std::is_array_v<StringOrChar>) {  //Для C-style массивов
                                                  // std::size() вернёт размер
                                                  //без учёта нуль-символа
    return std::size(val) > 0 ? std::size(val) - 1 : 0;
  } else if constexpr (std::is_pointer_v<StringOrChar>) {
    return details::null_terminated_string_length(val);
  } else if constexpr (!std::is_trivial_v<StringOrChar>) {
    return std::size(val);
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
constexpr details::internal_size_type<StrTy> find_substr(
    const StrTy& str,
    details::internal_size_type<StrTy> str_pos,
    const SubstrTy& substr) noexcept {
  auto str_length{details::internal_size_type<StrTy>(length(str) - str_pos)};
  return details::find_substr_impl(str, str_pos, str_length, substr,
                                   length(substr));
}

template <class StrTy, class SubstrTy>
constexpr size_t find_substr(const StrTy& str,
                             const SubstrTy& substr) noexcept {
  return find_substr(str, static_cast<details::internal_size_type<StrTy>>(0),
                     substr);
}

template <class StrTy, typename CharT>
constexpr details::internal_size_type<StrTy> find_character(
    const StrTy& str,
    details::internal_size_type<StrTy> str_pos,
    CharT ch) noexcept {
  return details::find_character_impl(str, str_pos, length(str) - str_pos, ch);
}

template <class StrTy, typename CharT>
constexpr details::internal_size_type<StrTy> find_character(const StrTy& str,
                                                            CharT ch) noexcept {
  return find_character(str, details::internal_size_type<StrTy>(0), ch);
}

namespace details {
template <class Container, class = void>
struct has_reserve : public std::false_type {};

template <class Container>
struct has_reserve<Container,
                   std::void_t<decltype(std::declval<Container>().reserve(
                       std::declval<size_t>()))>> : public std::true_type {};

template <class Container>
inline constexpr bool has_reserve_v = has_reserve<Container>::value;

template <class Container, class = void>
struct has_remove_prefix : public std::false_type {};

template <class Container>
struct has_remove_prefix<
    Container,
    std::void_t<decltype(std::declval<Container>().remove_prefix(
        std::declval<size_t>()))>> : public std::true_type {};

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

template <class CharT>
constexpr std::basic_string_view<CharT> make_basic_string_view(const CharT* ptr,
                                                               size_t length) {
  return std::basic_string_view<CharT>(ptr, length);
}
}  // namespace details

template <class OutputStrTy, class StrTy, class SubstrTy1, class SubstrTy2>
constexpr std::enable_if_t<details::has_remove_prefix_v<StrTy>, OutputStrTy>
replace_substr(StrTy str,  // string_view or string_ref
               const SubstrTy1& old_substr,
               const SubstrTy2& new_substr) {
  auto output_str{details::make_empty_string<OutputStrTy>(length(str))};
  const auto substr_len{length(old_substr)};

  while (true) {
    auto substr_pos{
        details::find_substr_impl(str, 0, length(str), old_substr, substr_len)};
    if (substr_pos == details::get_npos_v<StrTy>) {
      output_str += str.substr(0, length(str));
      break;
    }
    output_str += str.substr(0, substr_pos);
    output_str += new_substr;
    str.remove_prefix(substr_pos + length(old_substr));
  }
  return output_str;
}

template <class OutputStrTy, class StrTy, class SubstrTy1, class SubstrTy2>
constexpr std::enable_if_t<!details::has_remove_prefix_v<StrTy>, OutputStrTy>
replace_substr(const StrTy& str,
               const SubstrTy1& old_substr,
               const SubstrTy2& new_substr) {
  auto output_str{details::make_empty_string<OutputStrTy>(length(str))};
  const auto* data{std::data(str)};
  const auto str_len{length(str)};
  const auto substr_len{length(old_substr)};

  for (size_t start_pos = 0; start_pos < length(str);) {
    auto substr_pos{details::find_substr_impl(str, start_pos, str_len,
                                              old_substr, substr_len)};
    if (substr_pos == details::get_npos_v<StrTy>) {
      output_str += details::make_basic_string_view(data + start_pos,
                                                    length(str) - start_pos);
      break;
    }
    auto data_chunk_view{details::make_basic_string_view(
        data + start_pos, substr_pos - start_pos)};
    output_str += data_chunk_view;
    output_str += new_substr;
    start_pos += length(data_chunk_view) + length(old_substr);
  }
  return output_str;
}

template <class OutputString, class... Types>
OutputString concatenate(const Types&... strs_and_symbols) {
  OutputString result;
  if constexpr (details::has_reserve_v<OutputString>) {
    result.reserve(static_cast<details::internal_size_type<OutputString>>(
        summary_length(strs_and_symbols...)));
  }
  ((result += strs_and_symbols), ...);
  return result;
}

template <typename Number>
std::enable_if_t<std::is_integral_v<Number>, Number> from_string_view(
    std::string_view str) {  //Неактуально с появлением to_chars/from_chars
  char* p_end;
  return static_cast<Number>(std::strtoll(str.data(), &p_end, 10));
}

template <typename Number>
std::enable_if_t<std::is_floating_point_v<Number>, Number> from_string_view(
    std::string_view str) {  //Неактуально с появлением to_chars/from_chars
  char* p_end;
  return static_cast<Number>(std::strtod(str.data(), &p_end));
}

template <typename Number>
std::enable_if_t<std::is_arithmetic_v<Number>, std::string> number_to_string(
    Number number,
    std::optional<size_t> required_length = std::nullopt) {
  std::string result{std::to_string(number)};
  if (required_length) {
    if (*required_length <= result.length()) {
      result.resize(*required_length);
    } else {
      result.insert(0, *required_length - result.length(),
                    '0');  // Adding leading zeros
    }
  }
  return result;
}

template <class Predicate, class CharT, class Traits = std::char_traits<CharT>>
std::string_view strict(std::basic_string_view<CharT, Traits> str,
                        Predicate pred) {
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
  return std::memcmp(lhs, rhs, lhs_length);
}

template <typename CharT>
constexpr bool equal(CharT* lhs,
                     size_t lhs_length,
                     CharT* rhs,
                     size_t rhs_length) {
  return compare(lhs, lhs_length, rhs, rhs_length) == 0;
}

}  // namespace string
}  // namespace algo
