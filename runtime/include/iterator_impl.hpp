#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <iterator>
using std::distance;
using std::iterator_traits;
using std::next;
using std::prev;

// Iterator categories
using std::bidirectional_iterator_tag;
using std::forward_iterator_tag;
using std::input_iterator_tag;
using std::output_iterator_tag;
using std::random_access_iterator_tag;
#else
#include <functional_impl.hpp>
#include <memory_type_traits_impl.hpp>

namespace ktl {
struct input_iterator_tag {};
struct output_iterator_tag : input_iterator_tag {};
struct forward_iterator_tag : output_iterator_tag {};
struct bidirectional_iterator_tag : forward_iterator_tag {};
struct random_access_iterator_tag : bidirectional_iterator_tag {};

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

namespace it::details {
template <class Ty>
struct pointer_traits {
  using difference_type = ptrdiff_t;
  using value_type = Ty;
  using pointer = Ty*;
  using reference = Ty&;
  using iterator_category = random_access_iterator_tag;
  static constexpr bool is_raw_pointer = true;
  static constexpr bool is_fancy_pointer = false;
};
}  // namespace it::details

template <class Ty>
struct iterator_traits<Ty*> : it::details::pointer_traits<Ty> {};

template <class Ty>
struct iterator_traits<const Ty*> : it::details::pointer_traits<const Ty> {};

template <class Ty, size_t N>
struct iterator_traits<Ty[N]> : it::details::pointer_traits<Ty> {};

template <class Ty, size_t N>
struct iterator_traits<const Ty[N]> : it::details::pointer_traits<const Ty> {};

namespace it::details {
template <class It>
constexpr void advance_impl(
    It& it,
    typename iterator_traits<It>::difference_type offset,
    input_iterator_tag) {
  for (; offset > 0; --offset) {
    prefix_increment{}(it);
  };
}

template <class It>
constexpr void advance_impl(
    It& it,
    typename iterator_traits<It>::difference_type offset,
    bidirectional_iterator_tag) {
  for (; offset > 0; --offset) {
    prefix_increment{}(it);
  };
  for (; offset < 0; ++offset) {
    prefix_decrement{}(it);
  }
}

template <class It>
constexpr void advance_impl(
    It& it,
    typename iterator_traits<It>::difference_type offset,
    random_access_iterator_tag) {
  it += offset;
}
}  // namespace it::details

template <class InputIt, typename Distance>
constexpr void advance(InputIt& it, Distance offset) {
  using category = typename iterator_traits<InputIt>::iterator_category;
  auto distance{
      static_cast<typename iterator_traits<InputIt>::difference_type>(offset)};
  it::details::advance_impl(it, distance, category{});
}

namespace it::details {
template <class InputIt, typename Distance>
[[nodiscard]] constexpr InputIt advance_helper(InputIt it, Distance offset) {
  advance(it, offset);
  return it;
}
}  // namespace it::details

template <class InputIt>
[[nodiscard]] constexpr InputIt next(
    InputIt it,
    typename iterator_traits<InputIt>::difference_type offset = 1) {
  return it::details::advance_helper(it, offset);
}

template <class BidirectionalIt>
[[nodiscard]] constexpr BidirectionalIt prev(
    BidirectionalIt it,
    typename iterator_traits<BidirectionalIt>::difference_type offset = -1) {
  return it::details::advance_helper(it, offset);
}

namespace it::details {
template <class It>
[[nodiscard]] constexpr typename iterator_traits<It>::difference_type
distance_impl(It first, It last, random_access_iterator_tag) {
  return last - first;
}

template <class It>
[[nodiscard]] constexpr typename iterator_traits<It>::difference_type
distance_impl(It first, It last, input_iterator_tag) {
  using difference_type = typename iterator_traits<It>::difference_type;
  auto offset{static_cast<difference_type>(0)};
  for (; first != last; first = next(first)) {
    ++offset;
  }
  return offset;
}
}  // namespace it::details

template <class InputIt>
[[nodiscard]] constexpr typename iterator_traits<InputIt>::difference_type
distance(InputIt first, InputIt last) {
  using category = typename iterator_traits<InputIt>::iterator_category;
  return it::details::distance_impl(first, last, category{});
}
}  // namespace ktl
#endif