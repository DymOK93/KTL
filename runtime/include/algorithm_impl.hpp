#pragma once
#include <iterator_impl.hpp>

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
}  // namespace ktl