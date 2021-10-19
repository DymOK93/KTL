#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <algorithm>
namespace ktl {
using std::max;
using std::min;
}  // namespace ktl
#else
#include <algorithm_impl.hpp>
#include <memory.hpp>
#include <memory_type_traits.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

namespace ktl {
template <class LhsForwardIt, class RhsForwardIt>
constexpr void iter_swap(LhsForwardIt lhs, RhsForwardIt rhs) {
  swap(*lhs, *rhs);
}

template <class LhsInputIt, class RhsInputIt>
constexpr bool equal(LhsInputIt lhs_first,
                     LhsInputIt lhs_last,
                     RhsInputIt rhs_first) {
  return equal(lhs_first, lhs_last, rhs_first, equal_to<>{});
}

template <class LhsInputIt, class RhsInputIt, class BinaryPredicate>
constexpr bool equal(LhsInputIt lhs_first,
                     LhsInputIt lhs_last,
                     RhsInputIt rhs_first,
                     BinaryPredicate pred) {
  for (; lhs_first != lhs_last;
       lhs_first = next(lhs_first), rhs_first = next(rhs_first)) {
    if (!pred(*lhs_first, *rhs_first)) {
      return false;
    }
  }
  return true;
}

template <class LhsInputIt, class RhsInputIt>
constexpr bool equal(LhsInputIt lhs_first,
                     LhsInputIt lhs_last,
                     RhsInputIt rhs_first,
                     RhsInputIt rhs_last) {
  return equal(lhs_first, lhs_last, rhs_first, rhs_last, equal_to<>{});
}

namespace algo::details {
template <class LhsRandomAccessIt,
          class RhsRandomAccessIt,
          class BinaryPredicate>
constexpr bool equal_impl(LhsRandomAccessIt lhs_first,
                          LhsRandomAccessIt lhs_last,
                          RhsRandomAccessIt rhs_first,
                          RhsRandomAccessIt rhs_last,
                          BinaryPredicate pred,
                          true_type) {
  const auto lhs_length{static_cast<size_t>(lhs_last - lhs_first)},
      rhs_length{static_cast<size_t>(rhs_last - rhs_first)};
  if (lhs_length != rhs_length) {
    return false;
  }
  return equal(lhs_first, lhs_last, rhs_first, pred);
}

template <class LhsInputIt, class RhsInputIt, class BinaryPredicate>
constexpr bool equal_impl(LhsInputIt lhs_first,
                          LhsInputIt lhs_last,
                          RhsInputIt rhs_first,
                          RhsInputIt rhs_last,
                          BinaryPredicate pred,
                          false_type) {
  for (; lhs_first != lhs_last && rhs_first != rhs_last;
       lhs_first = next(lhs_first), rhs_first = next(rhs_first)) {
    if (!pred(*lhs_first, *rhs_first)) {
      return false;
    }
  }
  return true;
}
}  // namespace algo::details

template <class LhsInputIt, class RhsInputIt, class BinaryPredicate>
constexpr bool equal(LhsInputIt lhs_first,
                     LhsInputIt lhs_last,
                     RhsInputIt rhs_first,
                     RhsInputIt rhs_last,
                     BinaryPredicate pred) {
  constexpr bool random_access_iters{
      is_base_of_v<typename iterator_traits<LhsInputIt>::iterator_category,
                   random_access_iterator_tag> &&
      is_base_of_v<typename iterator_traits<RhsInputIt>::iterator_category,
                   random_access_iterator_tag>};

  return algo::details::equal_impl(lhs_first, lhs_last, rhs_first, rhs_last,
                                   pred, bool_constant<random_access_iters>{});
}

template <class InputIt, class Ty>
constexpr InputIt find(InputIt first, InputIt last, const Ty& value) {
  for (; first != last; first = next(first)) {
    if (*first == value) {
      break;
    }
  }
  return first;
}

// TODO: optimized version with memchr

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
template <class Ty>
Ty* copy_n_impl(const Ty* first, size_t count, Ty* d_first, true_type) {
  memmove(d_first, first, count * sizeof(Ty));
  return d_first + count;
}

template <class Ty>
Ty* copy_impl(const Ty* first, const Ty* last, Ty* d_first, true_type) {
  return copy_n_impl(first, static_cast<size_t>(last - first), d_first,
                     true_type{});
}

template <class InputIt, class OutputIt>
OutputIt copy_impl(InputIt first, InputIt last, OutputIt d_first, false_type) {
  for (; first != last; first = next(first), d_first = next(d_first)) {
    *d_first = *first;
  }
  return d_first;
}

template <class InputIt, class SizeType, class OutputIt>
OutputIt copy_n_impl(InputIt first,
                     SizeType count,
                     OutputIt d_first,
                     false_type) {
  for (auto idx = static_cast<SizeType>(0); idx != count;
       first = next(first), d_first = next(d_first), ++idx) {
    *d_first = *first;
  }
  return d_first;
}
}  // namespace algo::details

template <class InputIt, class OutputIt>
OutputIt copy(InputIt first, InputIt last, OutputIt d_first) {
  return algo::details::copy_impl(first, last, d_first,
                                  is_memcpyable_range<InputIt, OutputIt>{});
}

template <class InputIt, class SizeType, class OutputIt>
OutputIt copy_n(InputIt first, SizeType count, OutputIt d_first) {
  return algo::details::copy_n_impl(first, count, d_first,
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

template <class ForwardIt, class Ty>
void fill(ForwardIt first, ForwardIt last, const Ty& value) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;

  if constexpr (mm::details::wmemset_is_safe_v<value_type> &&
                is_pointer_v<ForwardIt>) {
    wmemset(first, static_cast<wchar_t>(value),
            static_cast<size_t>(last - first));
  } else {
    if (mm::details::is_memset_safe<value_type, ForwardIt>(value)) {
      memset(first, static_cast<int>(value),
             mm::details::distance_in_bytes(first, last));
    }
    for (; first != last; first = next(first)) {
      *first = value;
    }
  }
}

template <class OutputIt, class SizeType, class Ty>
OutputIt fill_n(OutputIt first, SizeType count, const Ty& value) {
  using value_type = typename iterator_traits<OutputIt>::value_type;

  if (count != static_cast<SizeType>(0)) {
    if constexpr (mm::details::wmemset_is_safe_v<value_type> &&
                  is_pointer_v<OutputIt>) {
      return wmemset(first, static_cast<wchar_t>(value),
                     static_cast<size_t>(count));
    } else {
      if (mm::details::is_memset_safe<value_type, OutputIt>(value)) {
        return static_cast<OutputIt>(
            memset(first, static_cast<int>(value), static_cast<size_t>(count)));
      }
      for (size_t idx = 0; idx < count; first = next(first), ++idx) {
        *first = value;
      }
    }
  }
  return first;
}

namespace algo::details {
template <class Ty>
Ty* shift_right_impl(Ty* first,
                     typename iterator_traits<Ty*>::difference_type offset,
                     size_t shift_count,
                     true_type) {
  auto* begin{first + offset};
  memmove(begin, first, shift_count * sizeof(Ty));
  return begin;
}

template <class ForwardIt>
ForwardIt shift_right_impl(
    ForwardIt first,
    typename iterator_traits<ForwardIt>::difference_type offset,
    size_t shift_count,
    false_type) {
  auto new_last{first}, current{first};
  advance(current, shift_count - 1);
  advance(new_last, offset + shift_count - 1);
  while (shift_count--) {
    *new_last = move(*current);
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
  const auto range_length{distance(first, last)};
  if (offset <= 0 || range_length < offset) {
    return first;
  }
  auto new_first{first};
  advance(new_first, offset);
  return move(new_first, last, first);
}

template <class ForwardIt>
ForwardIt shift_right(
    ForwardIt first,
    ForwardIt last,
    typename iterator_traits<ForwardIt>::difference_type offset) {
  const auto range_length{distance(first, last)};
  if (offset <= 0 || range_length < offset) {
    return last;
  }
  return algo::details::shift_right_impl(
      first, offset, static_cast<size_t>(range_length - offset),
      is_memcpyable<typename iterator_traits<ForwardIt>::value_type>{});
}

namespace algo::details {
template <class LhsInputIt, class RhsInputIt, class Compare, class = void>
struct LexicographicalComparator {
  constexpr bool operator()(LhsInputIt lhs_first,
                            LhsInputIt lhs_last,
                            RhsInputIt rhs_first,
                            RhsInputIt rhs_last,
                            Compare comp) const {
    for (; lhs_first != lhs_last && rhs_first != rhs_last;
         lhs_first = next(lhs_first), rhs_first = next(rhs_first)) {
      if (comp(*lhs_first, *rhs_first)) {
        return true;
      }
      if (comp(*rhs_first, *lhs_first)) {
        return false;
      }
    }
    return (lhs_first == lhs_last) && (rhs_first != rhs_last);
  }
};

template <class Ty, class UnaryPredicate, class LengthCompare>
bool compare_integral_impl(const Ty* lhs_first,
                           const Ty* lhs_last,
                           const Ty* rhs_first,
                           const Ty* rhs_last,
                           UnaryPredicate pred,
                           LengthCompare lc) {
  const ptrdiff_t first_length{lhs_last - lhs_first},
      second_length{rhs_last - rhs_first};
  const auto cmp_length{
      static_cast<size_t>((min)(first_length, second_length)) * sizeof(Ty)};

  const int result{memcmp(lhs_first, rhs_first, cmp_length)};
  if (result != 0) {
    return pred(result);
  }
  return lc(first_length, second_length);
}

template <class Ty>
bool compare_integral_less(const Ty* lhs_first,
                           const Ty* lhs_last,
                           const Ty* rhs_first,
                           const Ty* rhs_last) {
  return compare_integral_impl(
      lhs_first, lhs_last, rhs_first, rhs_last,
      [](int result) { return result < 0; }, less<>{});
}

template <class Ty>
bool compare_integral_greater(const Ty* lhs_first,
                              const Ty* lhs_last,
                              const Ty* rhs_first,
                              const Ty* rhs_last) {
  return compare_integral_impl<1>(
      lhs_first, lhs_last, rhs_first, rhs_last,
      [](int result) { return result > 0; }, greater<>{});
}

template <class Ty, class... Types>
struct LexicographicalComparator<Ty*,
                                 Ty*,
                                 less<Types...>,
                                 void_t<enable_if<is_integral_v<Ty> > > > {
  bool operator()(const Ty* lhs_first,
                  const Ty* lhs_last,
                  const Ty* rhs_first,
                  const Ty* rhs_last,
                  less<Types...>) const noexcept {
    return compare_integral_less(lhs_first, lhs_last, rhs_first, rhs_last);
  }
};

template <class Ty, class... Types>
struct LexicographicalComparator<Ty*,
                                 Ty*,
                                 greater<Types...>,
                                 void_t<enable_if<is_integral_v<Ty> > > > {
  bool operator()(const Ty* lhs_first,
                  const Ty* lhs_last,
                  const Ty* rhs_first,
                  const Ty* rhs_last,
                  greater<Types...>) const noexcept {
    return compare_integral_greater(lhs_first, lhs_last, rhs_first, rhs_last);
  }
};
}  // namespace algo::details

template <class LhsInputIt, class RhsInputIt>
bool lexicographical_compare(LhsInputIt lhs_first,
                             LhsInputIt lhs_last,
                             RhsInputIt rhs_first,
                             RhsInputIt rhs_last) {
  return lexicographical_compare(lhs_first, lhs_last, rhs_first, rhs_last,
                                 less<>{});
}

template <class LhsInputIt, class RhsInputIt, class Compare>
bool lexicographical_compare(LhsInputIt lhs_first,
                             LhsInputIt lhs_last,
                             RhsInputIt rhs_first,
                             RhsInputIt rhs_last,
                             Compare comp) {
  return algo::details::LexicographicalComparator<LhsInputIt, RhsInputIt,
                                                  Compare>{}(
      lhs_first, lhs_last, rhs_first, rhs_last, comp);
}

namespace algo::details {
template <class BidirectionalIt>
void reverse_impl(BidirectionalIt first,
                  BidirectionalIt last,
                  random_access_iterator_tag) {
  if (first != last) {
    for (last = prev(last); first < last;
         first = next(first), last = prev(last)) {
      iter_swap(first, last);
    }
  }
}

template <class BidirectionalIt>
void reverse_impl(BidirectionalIt first,
                  BidirectionalIt last,
                  bidirectional_iterator_tag) {
  while (first != last) {
    auto prev_last{prev(last)};
    if (first == prev_last) {
      break;
    }
    iter_swap(first, last);
    first = next(first);
  }
}
}  // namespace algo::details

template <class BidirectionalIt>
void reverse(BidirectionalIt first, BidirectionalIt last) {
  algo::details::reverse_impl(
      first, last,
      typename iterator_traits<BidirectionalIt>::iterator_category{});
}

namespace algo::details {
template <class RandomAccessIt>
RandomAccessIt rotate_left_impl(RandomAccessIt first,
                                RandomAccessIt new_first,
                                RandomAccessIt last,
                                random_access_iterator_tag) {
  reverse(first, new_first);
  reverse(new_first, last);
  reverse(first, last);
  return first + (last - new_first);
}

template <class BidirectionalIt>
BidirectionalIt rotate_left_impl(BidirectionalIt first,
                                 BidirectionalIt new_first,
                                 BidirectionalIt last,
                                 bidirectional_iterator_tag) {
  reverse(first, new_first);
  reverse(new_first, last);

  for (; first != new_first && last != new_first; first = next(first)) {
    last = prev(last);
    iter_swap(first, last);
  }

  reverse(first, last);

  return first;
}

template <class ForwardIt>
ForwardIt rotate_left_impl(ForwardIt first,
                           ForwardIt new_first,
                           ForwardIt last,
                           forward_iterator_tag) {
  auto current{new_first};

  do {
    iter_swap(first, current);
    first = next(first);
    current = next(current);
    if (first == new_first) {
      new_first = current;
    }
  } while (current != last);

  while (new_first != last) {
    current = new_first;
    do {
      iter_swap(first, current);
      first = next(first);
      current = next(current);
      if (first == new_first) {
        new_first = current;
      }
    } while (current != last);
  }

  return first;
}

template <class ForwardIt>
ForwardIt rotate_left(ForwardIt first, ForwardIt new_first, ForwardIt last) {
  if (first == new_first) {
    return new_first;
  }
  if (new_first == last) {
    return first;
  }
  return rotate_left_impl(
      first, new_first, last,
      typename iterator_traits<ForwardIt>::iterator_category{});
}
}  // namespace algo::details

template <class ForwardIt>
ForwardIt rotate(ForwardIt first, ForwardIt new_first, ForwardIt last) {
  return algo::details::rotate_left(first, new_first, last);
}

template <class LhsForwardIt, class RhsForwardIt>
constexpr RhsForwardIt swap_ranges(LhsForwardIt lhs_first,
                                   LhsForwardIt lhs_last,
                                   RhsForwardIt rhs_first) {
  for (; lhs_first != lhs_last;
       lhs_first = next(lhs_first), rhs_first = next(rhs_first)) {
    iter_swap(lhs_first, rhs_first);
  }
  return rhs_first;
}

template <class InputIt, class OutputIt, class UnaryOperation>
OutputIt transform(InputIt in_first,
                   InputIt in_last,
                   OutputIt out_first,
                   UnaryOperation unary_op) {
  for (; in_first != in_last;
       in_first = next(in_first), out_first = next(out_first)) {
    *out_first = unary_op(*in_first);
  }
  return out_first;
}

template <class LhsInputIt,
          class RhsInputIt,
          class OutputIt,
          class BinaryOperation>
OutputIt transform(LhsInputIt lhs_first,
                   LhsInputIt lhs_last,
                   RhsInputIt rhs_first,
                   OutputIt out_first,
                   BinaryOperation binary_op) {
  for (; lhs_first != lhs_last; lhs_first = next(lhs_first),
                                rhs_first = next(rhs_first),
                                out_first = next(out_first)) {
    *out_first = binary_op(*lhs_first, *rhs_first);
  }
  return out_first;
}
}  // namespace ktl
#endif
