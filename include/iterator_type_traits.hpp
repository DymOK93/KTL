#pragma once
#include <type_traits.hpp>
#include <functional.hpp>
#include <memory_type_traits.hpp>

namespace winapi::kernel::it::details {
template <class It>
struct is_random_access_iterator {
  static constexpr bool value =
      is_pointer_v<It> ||
      has_operator_plus_v<It, mm::details::get_size_type_t<It>> ||
      has_operator_plus_v<It, mm::details::get_difference_type_t<It>>;
};

template <class It>
inline constexpr bool is_random_access_iterator_v =
    is_random_access_iterator<It>::value;

}  // namespace winapi::kernel::it::details