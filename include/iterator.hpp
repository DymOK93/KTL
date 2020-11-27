#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <iterator>
using std::distance;
#else
#include <iterator_type_traits.hpp>
#include <functional.hpp>

namespace winapi::kernel {
template <class InputIt>
[[nodiscard]] constexpr mm::details::get_difference_type_t<InputIt> distance(
    InputIt first,
    InputIt last) {
  if constexpr (details::is_random_access_iterator_v<InputIt>) {
    return last - first;
  } else {
    using difference_type = mm::details::get_difference_type_t<InputIt>;
    auto offset{static_cast<difference_type>(0)};
    for (; first != last; ++first) {
      ++offset;
    }
    return offset;
  }
}

template <class InputIt>
[[nodiscard]] constexpr InputIt next(InputIt it) {
  if constexpr (has_operator_plus_v<InputIt,
                                    mm::details::get_size_type_t<InputIt> >) {
    return static_cast<InputIt>(  //
        plus<>{}(it, static_cast<mm::details::get_size_type_t<InputIt> >(1)));
  } else if constexpr (has_operator_plus_v<
                           InputIt,
                           mm::details::get_difference_type_t<InputIt> >) {
    return static_cast<InputIt>(plus<>{}(
        it, static_cast<mm::details::get_difference_type_t<InputIt> >(1)));
  } else if constexpr (has_prefix_increment_v<
                           add_lvalue_reference_t<InputIt> >) {
    return prefix_increment{}(it);
  } else if constexpr (has_postfix_increment_v<
                           add_lvalue_reference_t<InputIt> >) {
    return postfix_increment{}(it);
  } else {
    static_assert(always_false_v<InputIt>, "Iterator can't be increased");
  }
}  // namespace winapi::kernel::it

}  // namespace winapi::kernel
#endif