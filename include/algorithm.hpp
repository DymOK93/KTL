#pragma once

#ifdef KTL_NO_CXX_STANDARD_LIBRARY
#include <algorithm>
using std::min;
using std::max;
#else
#include <utility.hpp>
template <class Ty1, class Ty2>
constexpr decltype(auto) min(Ty1&& lhs,
                             Ty2&& rhs) noexcept(noexcept(forward<Ty2>(rhs) <
                                                          forward<Ty1>(lhs))) {
  return forward<Ty2>(rhs) < forward<Ty1>(lhs) ? forward<Ty2>(rhs)
                                               : forward<Ty1>(lhs);
}

template <class Ty1, class Ty2>
constexpr decltype(auto) max(Ty1&& lhs,
                             Ty2&& rhs) noexcept(noexcept(forward<Ty1>(lhs) <
                                                          forward<Ty2>(lhs))) {
  return forward<Ty1>(lhs) < forward<Ty2>(rhs) ? forward<Ty2>(rhs)
                                               : forward<Ty1>(lhs);
}
#endif

#include <iterator.hpp>
#include <utility.hpp>
#include <type_traits.hpp>

#ifndef KTL_CXX20
namespace ktl {
#ifndef KTL_NO_EXCEPTIONS
template <class ForwardIt>
ForwardIt
#else
template <class Verifier, class ForwardIt>
bool
#endif
shift_right(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    ForwardIt first,
    ForwardIt last,
    typename iterator_traits<ForwardIt>::difference_type offset) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;
  const size_t range_length{distance(first, last)};
  if (offset > 0 && offset < range_length) {
    if constexpr (is_trivially_copyable_v<value_type> &&
                  is_pointer_v<ForwardIt>) {
      auto* dest{first + offset * sizeof(value_type)};
      memmove(dest, first, (range_length - offset) * sizeof(value_type));
      return dest;
    } else {
      ForwardIt current{next(first)};
#ifndef KTL_NO_EXCEPTIONS
      while (current != last) {
#else
      bool successfully_assigned{true};
      while (current != last && successfully_assigned) {
#endif
        auto& place{*current};
        place = move(*first);
        current = next(current);
        first = next(first);
#ifdef KTL_NO_EXCEPTIONS
        successfully_assigned = vf(place);
#endif
      }
    }
#ifndef KTL_NO_EXCEPTIONS
    return first;
#else
    return successfully_assigned;
#endif
  }
#ifndef KTL_NO_EXCEPTIONS
  return last;
#else
  return false;
#endif
}

#ifndef KTL_NO_EXCEPTIONS
template <class ForwardIt>
ForwardIt
#else
template <class Verifier, class ForwardIt>
bool
#endif
shift_left(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    ForwardIt first,
    ForwardIt last,
    typename iterator_traits<ForwardIt>::difference_type offset) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;
  const size_t range_length{distance(first, last)};
  if (offset > 0 && offset < range_length) {
    if constexpr (is_trivially_copyable_v<value_type> &&
                  is_pointer_v<ForwardIt>) {
      auto* src{first + offset * sizeof(value_type)};
      memmove(first, src, (range_length - offset) * sizeof(value_type));
      return src;
    } else {
      ForwardIt current{prev(last)};
      ForwardIt old_current{current};
#ifndef KTL_NO_EXCEPTIONS
      while (current != first) {
#else
      bool successfully_assigned{true};
      while (current != first && successfully_assigned) {
#endif
        current = next(current);
        auto& place{*current};
        place = move(*old_current);
        old_current = current;
#ifdef KTL_NO_EXCEPTIONS
        successfully_assigned = vf(place);
#endif
      }
    }
#ifndef KTL_NO_EXCEPTIONS
    return old_current;
#else
    return successfully_assigned;
#endif
  }
#ifndef KTL_NO_EXCEPTIONS
  return first;
#else
  return false;
#endif
}
}  // namespace ktl
#endif