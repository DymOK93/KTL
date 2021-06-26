#pragma once
#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <algorithm>
namespace ktl {
using std::binary_search;
using std::max;
using std::min;
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
    if (pred(*middle, value)) {
      first = next(middle);
      count -= count_half + 1;
    } else {
      count = count_half;
    }
  }
  return (first == last) ? false : *first == value;
}

template <class ForwardIt, class Ty>
constexpr bool binary_search(ForwardIt first, ForwardIt last, const Ty& value) {
  return binary_search(first, last, value, less<>{});
}

template <class Ty1, class Ty2>
constexpr decltype(auto)(min)(const Ty1& lhs,
                              const Ty2& rhs) noexcept(noexcept(rhs < lhs)) {
  return rhs < lhs ? rhs : lhs;
}

template <class Ty1, class Ty2>
constexpr decltype(auto)(max)(const Ty1& lhs,
                              const Ty2& rhs) noexcept(noexcept(lhs < rhs)) {
  return lhs < rhs ? rhs : lhs;
}
}  // namespace ktl
#endif