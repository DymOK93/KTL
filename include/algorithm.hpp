#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <algorithm>
namespace ktl {
using std::max;
using std::min;
}  // namespace ktl
#else
#include <algorithm_impl.hpp>
#include <type_traits.hpp>
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

template <class InBidirectionalIt, class OutBidirectionalIt>
OutBidirectionalIt copy_backward(InBidirectionalIt first,
                                 InBidirectionalIt last,
                                 OutBidirectionalIt d_last) {
  for (; first != last; first = next(first)) {
    last = prev(last);
    d_last = prev(d_last);
    *d_last = *last;
  }
  return d_last;
}

namespace algo::details {
template <class InTy, class OutTy>
OutTy* copy_impl(InTy* first, InTy* last, OutTy* d_first, true_type) {
  return static_cast<OutTy*>(memmove(
      d_first, first, static_cast<size_t>(last - first) * sizeof(InTy)));
}

template <class InputIt, class OutputIt>
OutputIt copy_impl(InputIt first, InputIt last, OutputIt d_first, false_type) {
  for (; first != last; first = next(first), d_first = next(d_first)) {
    *d_first = *first;
  }
  return d_first;
}
}  // namespace algo::details

template <class InputIt, class OutputIt>
OutputIt copy(InputIt first, InputIt last, OutputIt d_first) {
  using input_value_type = typename iterator_traits<InputIt>::value_type;
  using output_value_type = typename iterator_traits<OutputIt>::value_type;

  return algo::details::copy_impl(first, last, d_first,
                                  is_memcpyable_range<InputIt, OutputIt>{});
}

template <class InputIt, class OutputIt>
OutputIt move(InputIt first, InputIt last, OutputIt d_first) {
  if constexpr (is_memcpyable_range_v<InputIt, OutputIt>) {
    return algo::details::copy_impl(first, last, d_first, true_type{});
  } else {
    for (; first != last; first = next(first), d_first = next(d_first)) {
      *d_first = move(*first);
    }
    return d_first;
  }
}

namespace algo::details {
template <class ForwardIt, class Ty>
struct Filler {
  ForwardIt
  operator()(ForwardIt first, ForwardIt last, const Ty& value) const noexcept(
      is_nothrow_assignable_v<typename iterator_traits<ForwardIt>::value_type,
                              Ty>) {
    for (; first != last; first = next(first)) {
      *first = value;
    }
  }
};

template <>
struct Filler<char*, char> {
  char* operator()(char* first, char* last, char value) const noexcept {
    return static_cast<char*>(
        memset(first, value, static_cast<size_t>(last - first)));
  }
};

template <>
struct Filler<wchar_t*, wchar_t> {
  wchar_t* operator()(wchar_t* first,
                      wchar_t* last,
                      wchar_t value) const noexcept {
    return wmemset(first, value, static_cast<size_t>(last - first));
  }
};
}  // namespace algo::details

template <class ForwardIt, class Ty>
void fill(ForwardIt first, ForwardIt last, const Ty& value) {
  return algo::details::Filler<ForwardIt, Ty>{}(first, last, value);
}

namespace algo::details {
template <class ForwardIt, class Shifter>
ForwardIt shift_impl(
    ForwardIt first,
    ForwardIt last,
    typename iterator_traits<ForwardIt>::difference_type offset,
    Shifter shifter) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;
  using difference_type = typename iterator_traits<ForwardIt>::difference_type;

  const auto range_length{distance(first, last)};
  if (offset > 0 && offset < range_length) {
    return shifter(first, offset, range_length - offset,
                   is_memcpyable_range<ForwardIt, ForwardIt>{});
  }
  return last;
}

template <class Ty>
Ty* shift_left_impl(Ty* first,
                    typename iterator_traits<Ty*>::difference_type offset,
                    size_t count,
                    true_type) {
  return static_cast<Ty*>(memmove(first, first + offset, count * sizeof(Ty)));
}

template <class ForwardIt>
ForwardIt shift_left_impl(
    ForwardIt first,
    typename iterator_traits<ForwardIt>::difference_type offset,
    size_t count,
    false_type) {
  auto new_first{first};
  advance(new_first, offset);
  while (count--) {
    *first = *new_first;
    first = next(first);
    new_first = next(new_first);
  }
  return new_first;
}

template <class Ty>
Ty* shift_right_impl(Ty* first,
                     typename iterator_traits<Ty*>::difference_type offset,
                     size_t count,
                     true_type) {
  return static_cast<Ty*>(memmove(first + offset, first, count * sizeof(Ty)));
}

template <class ForwardIt>
ForwardIt shift_right_impl(
    ForwardIt first,
    typename iterator_traits<ForwardIt>::difference_type offset,
    size_t count,
    false_type) {
  auto new_last{first}, current{first};
  advance(current, count - 1);
  advance(new_last, offset + count - 1);
  while (count--) {
    *new_last = *current;
    new_last = prev(new_last);
    current = prev(current);
  }
  return new_last;
}
}  // namespace algo::details

template <class ForwardIt>
ForwardIt shift_left(
    ForwardIt first,
    ForwardIt last,
    typename iterator_traits<ForwardIt>::difference_type offset) {
  return algo::details::shift_impl(first, last, offset, [](auto... args) {
    return algo::details::shift_left_impl(args...);
  });
}

template <class ForwardIt>
ForwardIt shift_right(
    ForwardIt first,
    ForwardIt last,
    typename iterator_traits<ForwardIt>::difference_type offset) {
  return algo::details::shift_impl(first, last, offset, [](auto... args) {
    return algo::details::shift_right_impl(args...);
  });
}
}  // namespace ktl
#endif
