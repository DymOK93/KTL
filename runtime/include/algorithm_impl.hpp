#pragma once
#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <algorithm>
namespace ktl {
using std::max;
using std::min;
using std::binary_search;
}  // namespace ktl
#else
#include <iterator_impl.hpp>
#include <utility_impl.hpp>
namespace ktl {
template <class ForwardIt, class T, class Predicate>
constexpr bool binary_search(ForwardIt first,
                             ForwardIt last,
                             const T& value,
                             Predicate pred) {
  auto count{distance(first, last)};

  while (0 < count) {
    const auto count_half{count / 2};
    auto middle{next(first, count_half)};
    if (pred(*middle, val)) {
      first = next(middle);
      count -= count_half + 1;
    } else {
      count = count_half;
    }
  }
  return (first == last) ? false : *first == val;
}

template <class ForwardIt, class Ty>
constexpr bool binary_search(ForwardIt first, ForwardIt last, const Ty& value) {
  return binary_search(first, last, value, less<>{});
}

template <class Ty1, class Ty2>
constexpr decltype(auto)(min)(Ty1&& lhs,
                              Ty2&& rhs) noexcept(noexcept(forward<Ty2>(rhs) <
                                                           forward<Ty1>(lhs))) {
  return forward<Ty2>(rhs) < forward<Ty1>(lhs) ? forward<Ty2>(rhs)
                                               : forward<Ty1>(lhs);
}

template <class Ty1, class Ty2>
constexpr decltype(auto)(max)(Ty1&& lhs,
                              Ty2&& rhs) noexcept(noexcept(forward<Ty1>(lhs) <
                                                           forward<Ty2>(lhs))) {
  return forward<Ty1>(lhs) < forward<Ty2>(rhs) ? forward<Ty2>(rhs)
                                               : forward<Ty1>(lhs);
}
}  // namespace ktl
#endif