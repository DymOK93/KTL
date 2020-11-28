#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <iterator>
using std::distance;
using std::prev;
using std::next;
using std::iterator_traits;

// Iterator categories
using std::input_iterator_tag;
using std::output_iterator_tag;
using std::forward_iterator_tag;
using std::bidirectional_iterator_tag;
using std::random_access_iterator_tag;
#else
#include <memory_type_traits.hpp>
#include <functional.hpp>

namespace winapi::kernel {
struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag {};
struct bidirectional_iterator_tag {};
struct random_access_iterator_tag {};

namespace it::details {
template <class It, class = void>
struct get_reference_type {
  using type = decltype(*declval<It>());
};

template <class It>
struct get_reference_type<It, void_t<typename It::value_type> > {
  using type = typename It::reference;
};

template <class It>
using get_reference_type_t = typename get_reference_type<It>::type;

template <class It, class = void>
struct get_value_type {
  using type = remove_reference_t<get_reference_type_t<It> >;
};

template <class It>
struct get_value_type<It, void_t<typename It::value_type> > {
  using type = typename It::value_type;
};

template <class It>
using get_value_type_t = typename get_value_type<It>::type;

template <class It, class = void>
struct get_pointer_type {
  using type = add_pointer_t<get_value_type_t<It> >;
};

template <class It>
struct get_pointer_type<It, void_t<typename It::value_type> > {
  using type = typename It::pointer;
};

template <class It>
using get_pointer_type_t = typename get_pointer_type<It>::type;
}  // namespace it::details

template <class It>
struct iterator_traits {
  using difference_type = mm::details::get_difference_type_t<It>;
  using value_type = it::details::get_value_type_t<It>;
  using pointer = it::details::get_pointer_type_t<It>;
  using reference = it::details::get_reference_type_t<It>;
  using iterator_category = typename It::iterator_category;
  static constexpr bool is_raw_pointer = false;
  static constexpr bool is_fancy_pointer = true;
};

template <class Ty>
struct iterator_traits<Ty*> {
  using difference_type = ptrdiff_t;
  using value_type = Ty;
  using pointer = Ty*;
  using reference = Ty&;
  using iterator_category = random_access_iterator_tag;
  static constexpr bool is_raw_pointer = true;
  static constexpr bool is_fancy_pointer = false;
};

template <class InputIt>
[[nodiscard]] constexpr typename iterator_traits<InputIt>::difference_type
distance(InputIt first, InputIt last) {
  if constexpr (details::is_random_access_iterator_v<InputIt>) {
    return last - first;
  } else {
    using difference_type = typename iterator_traits<InputIt>::difference_type;
    auto offset{static_cast<difference_type>(0)};
    for (; first != last; ++first) {
      ++offset;
    }
    return offset;
  }
}

template <class InputIt>
[[nodiscard]] constexpr InputIt next(InputIt it) {
  if constexpr (has_operator_plus_v<InputIt, typename iterator_traits<
                                                 InputIt>::difference_type>) {
    return plus<>{}(
        it,
        static_cast<typename iterator_traits<InputIt>::difference_type>(1));
  } else if constexpr (has_operator_minus_v<InputIt,
                                           typename iterator_traits<
                                               InputIt>::difference_type>) {
    return minus<>{}(
        it, static_cast<typename iterator_traits<InputIt>::difference_type>(-1));
  }
   else if constexpr (has_prefix_increment_v<
                           add_lvalue_reference_t<InputIt> >) {
    return prefix_increment{}(it);
  } else if constexpr (has_postfix_increment_v<
                           add_lvalue_reference_t<InputIt> >) {
    return postfix_increment{}(it);
  } else {
    static_assert(always_false_v<InputIt>, "Iterator can't be increased");
  }
}

template <class BidirectionalIt>
[[nodiscard]] constexpr BidirectionalIt prev(BidirectionalIt it) {
  if constexpr (has_operator_plus_v<InputIt, typename iterator_traits<
                                                 InputIt>::difference_type>) {
    return plus<>{}(
        it, static_cast<typename iterator_traits<InputIt>::difference_type>(-1));
  } else if constexpr (has_operator_minus_v<InputIt,
                                            typename iterator_traits<
                                                InputIt>::difference_type>) {
    return minus<>{}(
        it,
        static_cast<typename iterator_traits<InputIt>::difference_type>(1));
  } else if constexpr (has_prefix_decrement_v<
                           add_lvalue_reference_t<InputIt> >) {
    return prefix_decrement{}(it);
  } else if constexpr (has_postfix_decrement_v<
                           add_lvalue_reference_t<InputIt> >) {
    return postfix_decrement{}(it);
  } else {
    static_assert(always_false_v<InputIt>, "Iterator can't be increased");
  }
}

}  // namespace winapi::kernel
#endif