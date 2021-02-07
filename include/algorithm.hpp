#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <algorithm>
namespace ktl {
using std::max;
using std::min;
}  // namespace ktl
#else
#include <algorithm_impl.hpp>
#include <utility.hpp>
namespace ktl {
template <class InputIt, class Ty>
constexpr InputIt find(InputIt first, InputIt last, const Ty& value) {
  for (; first != last; first = next(first)) {
    if (*first == value) {
      break;
    }
  }
  return first;
}

template <class InputIt, class UnaryPredicate>
constexpr InputIt find_if(InputIt first, InputIt last, UnaryPredicate pred) {
  for (; first != last; first = next(first)) {
    if (pred(*first)) {
      break;
    }
  }
  return first;
}

template <class InputIt, class UnaryPredicate>
constexpr InputIt find_if_not(InputIt first,
                              InputIt last,
                              UnaryPredicate pred) {
  return find_if(first, last, [pred](auto&& arg) {
    return !pred(forward<decltype(arg)>(arg));
  });
}

namespace algo::details {
template <class ForwardIt, class InputIt, class Comparator>
constexpr pair<ForwardIt, size_t> count_matches_at_begin(ForwardIt range_first,
                                                         InputIt subrange_first,
                                                         InputIt subrange_last,
                                                         Comparator comp) {
  size_t matches_count{0};
  while (subrange_first != subrange_last &&
         comp(*range_first, *subrange_first)) {
    range_first = next(range_first);
    subrange_first = next(subrange_first);
    ++matches_count;
  }
  return {range_first, matches_count};
}
}  // namespace algo::details

template <class ForwardIt1, class ForwardIt2, class Comparator>
constexpr ForwardIt1 find_subrange(ForwardIt1 range_first,
                                   ForwardIt1 range_last,
                                   ForwardIt2 subrange_first,
                                   ForwardIt2 subrange_last,
                                   Comparator comp) {
  auto range_length{static_cast<size_t>(distance(range_first, range_last))};
  const auto subrange_length{
      static_cast<size_t>(distance(subrange_first, subrange_last))};
  while (range_length >= subrange_length) {
    auto [last_maches, matches_count]{algo::details::count_matches_at_begin(
        range_first, subrange_first, subrange_last, comp)};
    if (matches_count == subrange_length) {
      return range_first;
    }
    range_first = next(last_maches);
    range_length -= (matches_count + 1);
  }
  return range_last;
}
}  // namespace ktl
#endif

//#include <iterator.hpp>
//#include <utility.hpp>
//#include <type_traits.hpp>

//#ifndef KTL_CXX20
// namespace ktl {
//#ifndef KTL_NO_EXCEPTIONS
// template <class ForwardIt>
// ForwardIt
//#else
// template <class Verifier, class ForwardIt>
// bool
//#endif
// shift_right(
//#ifdef KTL_NO_EXCEPTIONS
//    Verifier&& vf,
//#endif
//    ForwardIt first,
//    ForwardIt last,
//    typename iterator_traits<ForwardIt>::difference_type offset) {
//  using value_type = typename iterator_traits<ForwardIt>::value_type;
//  const size_t range_length{distance(first, last)};
//  if (offset > 0 && offset < range_length) {
//    if constexpr (is_trivially_copyable_v<value_type> &&
//                  is_pointer_v<ForwardIt>) {
//      auto* dest{first + offset * sizeof(value_type)};
//      memmove(dest, first, (range_length - offset) * sizeof(value_type));
//      return dest;
//    } else {
//      ForwardIt current{next(first)};
//#ifndef KTL_NO_EXCEPTIONS
//      while (current != last) {
//#else
//      bool successfully_assigned{true};
//      while (current != last && successfully_assigned) {
//#endif
//        auto& place{*current};
//        place = move(*first);
//        current = next(current);
//        first = next(first);
//#ifdef KTL_NO_EXCEPTIONS
//        successfully_assigned = vf(place);
//#endif
//      }
//    }
//#ifndef KTL_NO_EXCEPTIONS
//    return first;
//#else
//    return successfully_assigned;
//#endif
//  }
//#ifndef KTL_NO_EXCEPTIONS
//  return last;
//#else
//  return false;
//#endif
//}
//
//#ifndef KTL_NO_EXCEPTIONS
// template <class ForwardIt>
// ForwardIt
//#else
// template <class Verifier, class ForwardIt>
// bool
//#endif
// shift_left(
//#ifdef KTL_NO_EXCEPTIONS
//    Verifier&& vf,
//#endif
//    ForwardIt first,
//    ForwardIt last,
//    typename iterator_traits<ForwardIt>::difference_type offset) {
//  using value_type = typename iterator_traits<ForwardIt>::value_type;
//  const size_t range_length{distance(first, last)};
//  if (offset > 0 && offset < range_length) {
//    if constexpr (is_trivially_copyable_v<value_type> &&
//                  is_pointer_v<ForwardIt>) {
//      auto* src{first + offset * sizeof(value_type)};
//      memmove(first, src, (range_length - offset) * sizeof(value_type));
//      return src;
//    } else {
//      ForwardIt current{prev(last)};
//      ForwardIt old_current{current};
//#ifndef KTL_NO_EXCEPTIONS
//      while (current != first) {
//#else
//      bool successfully_assigned{true};
//      while (current != first && successfully_assigned) {
//#endif
//        current = next(current);
//        auto& place{*current};
//        place = move(*old_current);
//        old_current = current;
//#ifdef KTL_NO_EXCEPTIONS
//        successfully_assigned = vf(place);
//#endif
//      }
//    }
//#ifndef KTL_NO_EXCEPTIONS
//    return old_current;
//#else
//    return successfully_assigned;
//#endif
//  }
//#ifndef KTL_NO_EXCEPTIONS
//  return first;
//#else
//  return false;
//#endif
//}
//}  // namespace ktl
//#endif