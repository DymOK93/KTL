#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <memory>
#include <utility>
#else
#include <heap.h>
#include <new_delete.h>
#include <allocator.hpp>
#include <iterator.hpp>
#include <memory_impl.hpp>
#include <memory_type_traits.hpp>
#include <smart_pointer.hpp>
#include <type_traits.hpp>
#include <utility.hpp>
#endif

namespace ktl {
#ifndef KTL_NO_CXX_STANDARD_LIBRARY
// there is no std::construct_at in C++17
using std::destroy;
using std::destroy_n;
using std::launder;
using std::uninitialized_copy;
using std::uninitialized_copy_n;
using std::uninitialized_default_construct;
using std::uninitialized_default_construct_n;
using std::uninitialized_fill;
using std::uninitialized_fill_n;
using std::uninitialized_move;
using std::uninitialized_move_n;
using std::uninitialized_value_construct;
using std::uninitialized_value_construct_n;
#else

template <class Ty>
[[nodiscard]] constexpr Ty* launder(Ty* ptr) noexcept {
  static_assert(!is_function_v<Ty> && !is_void_v<Ty>,
                "N4727 21.6.4 [ptr.launder]/3: The program is ill-formed if Ty "
                "is a function type or cv void");
  return __builtin_launder(ptr);
}

template <class InputIt>
constexpr void destroy(
    InputIt first,
    InputIt last) noexcept {  // CRASH if d-tor isn't noexcept
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (!is_trivially_destructible_v<value_type>) {
    for (; first != last; first = next(first)) {
      destroy_at(addressof(*first));
    }
  }
}

template <class InputIt, class Allocator>
constexpr void destroy(
    InputIt first,
    InputIt last,
    Allocator& alloc) noexcept {  // CRASH if d-tor isn't noexcept
  using value_type = typename iterator_traits<InputIt>::value_type;
  using allocator_type = Allocator;
  using allocator_traits_type = allocator_traits<allocator_type>;

  if constexpr (!is_trivially_destructible_v<value_type> ||
                mm::details::has_destroy_v<
                    Allocator, typename iterator_traits<InputIt>::pointer>) {
    for (; first != last; first = next(first)) {
      allocator_traits_type::destroy(alloc, addressof(*first));
    }
  }
}

template <class InputIt>
constexpr void destroy_n(
    InputIt first,
    size_t count) noexcept {  // CRASH if d-tor isn't noexcept
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (!is_trivially_destructible_v<value_type>) {
    for (size_t step = 0; step < count; first = next(first), ++step) {
      destroy_at(addressof(*first));
    }
  }
}

template <class InputIt, class Allocator>
constexpr void destroy_n(
    InputIt first,
    size_t count,
    Allocator& alloc) noexcept {  // CRASH if d-tor isn't noexcept
  using value_type = typename iterator_traits<InputIt>::value_type;
  using allocator_type = Allocator;
  using allocator_traits_type = allocator_traits<allocator_type>;

  if constexpr (!is_trivially_destructible_v<value_type> ||
                mm::details::has_destroy_v<Allocator,
                                           decltype(addressof(*first))>) {
    for (size_t step = 0; step < count; first = next(first), ++step) {
      allocator_traits_type::destroy(alloc, addressof(*first));
    }
  }
}

namespace mm::details {
template <class Ty>
constexpr size_t distance_in_bytes(Ty* first, Ty* last) noexcept {
  return static_cast<size_t>(last - first) * sizeof(Ty);
}

template <class InputTy, class OutputTy>
OutputTy* uninitialized_copy_trivial_impl(InputTy* first,
                                          InputTy* last,
                                          OutputTy* dest) {
  return static_cast<OutputTy*>(
      memmove(dest, first, distance_in_bytes(first, last)));
}

template <class InputTy, class OutputTy>
OutputTy* uninitialized_copy_trivial_impl(InputTy* first,
                                          size_t count,
                                          OutputTy* dest) {
  return static_cast<OutputTy*>(memmove(dest, first, count * sizeof(InputTy)));
}

template <class InputTy, class OutputTy>
OutputTy* uninitialized_copy_unchecked_trivial_impl(InputTy* first,
                                                    InputTy* last,
                                                    OutputTy* dest) {
  return static_cast<OutputTy*>(
      memcpy(dest, first, distance_in_bytes(first, last)));
}

template <class InputTy, class OutputTy>
OutputTy* uninitialized_copy_unchecked_trivial_impl(InputTy* first,
                                                    size_t count,
                                                    OutputTy* dest) {
  return static_cast<OutputTy*>(memcpy(dest, first, count * sizeof(InputTy)));
}

template <class ForwardIt>
class uninitialized_backout_base {
 public:
  uninitialized_backout_base(ForwardIt dst)
      : m_dst_first{dst}, m_dst_cur{dst} {}

  ForwardIt release() {
    m_dst_first = m_dst_cur;
    return m_dst_cur;
  }

 protected:
  ForwardIt m_dst_first;
  ForwardIt m_dst_cur;
};

template <class ForwardIt>
class uninitialized_backout : public uninitialized_backout_base<ForwardIt> {
 public:
  using MyBase = uninitialized_backout_base<ForwardIt>;

 public:
  using MyBase::MyBase;
  ~uninitialized_backout() { destroy(this->m_dst_first, this->m_dst_cur); }

 protected:
  template <class... Types>
  decltype(auto) emplace(Types&&... args) {
    auto& dst{this->m_dst_cur};
    auto& place{*construct_at(addressof(*dst), forward<Types>(args)...)};
    dst = next(dst);
    return place;
  }

  decltype(auto) value_construct() { return emplace(); }

  decltype(auto) default_construct() {
    auto* place{this->m_dst_cur++};
    default_construct_at(place);
    return *place;
  }
};

template <class ForwardIt, class Allocator>
class uninitialized_backout_with_allocator
    : public uninitialized_backout_base<ForwardIt> {
 public:
  using MyBase = uninitialized_backout_base<ForwardIt>;
  using allocator_type = Allocator;
  using allocator_traits_type = allocator_traits<allocator_type>;

 public:
  uninitialized_backout_with_allocator(ForwardIt dst, Allocator& alc)
      : MyBase(dst), m_alc{addressof(alc)} {}

  ~uninitialized_backout_with_allocator() {
    destroy(this->m_dst_first, this->m_dst_cur, *m_alc);
  }

 protected:
  template <class... Types>
  decltype(auto) emplace(Types&&... args) {
    auto& dst{this->m_dst_cur};
    auto& place{allocator_traits_type::construct(addressof(*dst),
                                                 forward<Types>(args)...)};
    dst = next(dst);
    return place;
  }

  decltype(auto) value_construct() { return emplace(); }
  decltype(auto) default_construct() { return emplace(); }

 private:
  Allocator* const m_alc;
};

template <class Backout>
struct uninitialized_emplace_helper : public Backout {
  using MyBase = Backout;

  using MyBase::MyBase;

  template <class InputIt>
  void emplace_range(InputIt first, InputIt last) {
    for (; first != last; first = next(first)) {
      MyBase::emplace(*first);
    }
  }

  template <class InputIt>
  void emplace_n(InputIt first, size_t count) {
    for (size_t step = 0; step != count; first = next(first), ++step) {
      MyBase::emplace(*first);
    }
  }

  template <class InputIt>
  void move_range(InputIt first, InputIt last) {
    for (; first != last; first = next(first)) {
      MyBase::emplace(move(*first));
    }
  }

  template <class InputIt>
  void move_n(InputIt first, size_t count) {
    for (size_t step = 0; step != count; first = next(first), ++step) {
      MyBase::emplace(move(*first));  // InputIt may be move iterator
    }
  }
};

template <class ForwardIt>
using emplace_helper =
    uninitialized_emplace_helper<uninitialized_backout<ForwardIt> >;

template <class ForwardIt, class Allocator>
using emplace_helper_with_allocator = uninitialized_emplace_helper<
    uninitialized_backout_with_allocator<ForwardIt, Allocator> >;

template <template <class... Types> class EmplaceHelper,
          class InputIt,
          class NoThrowForwardIt,
          class... Allocator>
NoThrowForwardIt uninitialized_copy_non_trivial_impl(InputIt first,
                                                     InputIt last,
                                                     NoThrowForwardIt dest,
                                                     Allocator&... allocs) {
  EmplaceHelper<NoThrowForwardIt, Allocator> uemph(dest, allocs...);
  uemph.emplace_range(first, last);
  return uemph.release();
}

template <template <class... Types> class EmplaceHelper,
          class InputIt,
          class NoThrowForwardIt,
          class... Allocator>
NoThrowForwardIt uninitialized_copy_n_non_trivial_impl(InputIt first,
                                                       size_t count,
                                                       NoThrowForwardIt dest,
                                                       Allocator&... allocs) {
  EmplaceHelper<NoThrowForwardIt, Allocator...> uemph(dest, allocs...);
  uemph.emplace_n(first, count);
  return uemph.release();
}

template <template <class... Types> class EmplaceHelper,
          class InputIt,
          class NoThrowForwardIt,
          class... Allocator>
NoThrowForwardIt uninitialized_move_impl(InputIt first,
                                         InputIt last,
                                         NoThrowForwardIt dest,
                                         Allocator&... allocs) {
  EmplaceHelper<NoThrowForwardIt, Allocator...> uemph(dest, allocs...);
  uemph.move_range(first, last);
  return uemph.release();
}

template <template <class... Types> class EmplaceHelper,
          class InputIt,
          class NoThrowForwardIt,
          class... Allocators>
NoThrowForwardIt uninitialized_move_n_impl(InputIt first,
                                           size_t count,
                                           NoThrowForwardIt dest,
                                           Allocators&... allocs) {
  EmplaceHelper<NoThrowForwardIt, Allocators...> uemph(dest, allocs...);
  uemph.move_n(first, count);
  return uemph.release();
}

template <class Ty, class InputIt, class OutputIt, class Allocator>
inline constexpr bool memcpy_or_memmove_is_safe =
    (is_trivially_copyable_v<Ty>)&&(is_pointer_v<InputIt>)&&(
        is_pointer_v<
            OutputIt>)&&(is_same_v<remove_cv_t<typename iterator_traits<InputIt>::
                                                   value_type>,
                                   remove_cv_t<typename iterator_traits<
                                       OutputIt>::value_type> >);

template <class Ty,
          class InputIt,
          class OutputIt,
          class Allocator,
          class... Types>
inline constexpr bool enable_transfer_without_allocator_v =
    memcpy_or_memmove_is_safe_v<Ty, InputIt, OutputIt> &&
    !has_construct_v<Allocator,
                     typename iterator_traits<OutputIt>::pointer,
                     typename iterator_traits<InputIt>::reference>;
}  // namespace mm::details

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt uninitialized_copy(InputIt first,
                                    InputIt last,
                                    NoThrowForwardIt dest) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {  // using memmove() for
                                                   // trivially copyable types
    return mm::details::uninitialized_copy_trivial_impl(first, last, dest);
  } else {
    return mm::details::uninitialized_copy_non_trivial_impl<
        mm::details::emplace_helper>(first, last, dest);
  }
}

template <class InputIt, class NoThrowForwardIt, class Allocator>
NoThrowForwardIt uninitialized_copy(InputIt first,
                                    InputIt last,
                                    NoThrowForwardIt dest,
                                    Allocator& alloc) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (mm::details::enable_transfer_without_allocator_v<
                    value_type, InputIt, NoThrowForwardIt,
                    Allocator>) {  // using memmove() for
                                   // trivially copyable types
    return mm::details::uninitialized_copy_trivial_impl(first, last, dest);
  } else {
    return mm::details::uninitialized_copy_non_trivial_impl<
        mm::details::emplace_helper_with_allocator>(first, last, dest, alloc);
  }
}

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt uninitialized_copy_unchecked(InputIt first,
                                              InputIt last,
                                              NoThrowForwardIt dest) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {  // using memcpy() for
                                                   // trivially copyable types
    return mm::details::uninitialized_copy_unchecked_trivial_impl(
        first, last,  // always true
        dest);
  } else {
    return mm::details::uninitialized_copy_non_trivial_impl<
        mm::details::emplace_helper>(first, last, dest);
  }
}

template <class InputIt, class NoThrowForwardIt, class Allocator>
NoThrowForwardIt uninitialized_copy_unchecked(InputIt first,
                                              InputIt last,
                                              NoThrowForwardIt dest,
                                              Allocator& alloc) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (mm::details::enable_transfer_without_allocator_v<
                    value_type, InputIt, NoThrowForwardIt,
                    Allocator>) {  // using memcpy() for
                                   // trivially copyable types
    return mm::details::uninitialized_copy_unchecked_trivial_impl(
        first, last,  // always true
        dest);
  } else {
    return mm::details::uninitialized_copy_non_trivial_impl<
        mm::details::emplace_helper_with_allocator>(first, last, dest, alloc);
  }
}

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt uninitialized_copy_n(InputIt first,
                                      size_t count,
                                      NoThrowForwardIt dest) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {  // using memmove() for
                                                   // trivially copyable types
    return mm::details::uninitialized_copy_trivial_impl(first, count, dest);
  } else {
    return mm::details::uninitialized_copy_n_non_trivial_impl<
        mm::details::emplace_helper>(first, count, dest);
  }
}

template <class InputIt, class NoThrowForwardIt, class Allocator>
NoThrowForwardIt uninitialized_copy_n(InputIt first,
                                      size_t count,
                                      NoThrowForwardIt dest,
                                      Allocator& alloc) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (mm::details::enable_transfer_without_allocator_v<
                    value_type, InputIt, NoThrowForwardIt,
                    Allocator>) {  // using memmove() for
                                   // trivially copyable types
    return mm::details::uninitialized_copy_trivial_impl(first, count, dest);
  } else {
    return mm::details::uninitialized_copy_n_non_trivial_impl<
        mm::details::emplace_helper_with_allocator>(first, count, dest, alloc);
  }
}

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt uninitialized_copy_n_unchecked(InputIt first,
                                                size_t count,
                                                NoThrowForwardIt dest) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {  // using memcpy() for
                                                   // trivially copyable types
    return mm::details::uninitialized_copy_unchecked_trivial_impl(first, count,
                                                                  dest);
  } else {
    return mm::details::uninitialized_copy_n_non_trivial_impl<
        mm::details::emplace_helper>(first, count, dest);
  }
}

template <class InputIt, class NoThrowForwardIt, class Allocator>
NoThrowForwardIt uninitialized_copy_n_unchecked(InputIt first,
                                                size_t count,
                                                NoThrowForwardIt dest,
                                                Allocator& alloc) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (mm::details::enable_transfer_without_allocator_v<
                    value_type, InputIt, NoThrowForwardIt,
                    Allocator>) {  // using memcpy() for
                                   // trivially copyable types
    return mm::details::uninitialized_copy_unchecked_trivial_impl(first, count,
                                                                  dest);
  } else {
    return mm::details::uninitialized_copy_n_non_trivial_impl<
        mm::details::emplace_helper_with_allocator>(first, count, dest, alloc);
  }
}

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt uninitialized_move(InputIt first,
                                    InputIt last,
                                    NoThrowForwardIt dest) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {  // using memmove() for
                                                   // trivially copyable types

    return mm::details::uninitialized_copy_trivial_impl(first, last, dest);
  } else {
    return mm::details::uninitialized_move_impl<mm::details::emplace_helper>(
        first, last, dest);
  }
}

template <class InputIt, class NoThrowForwardIt, class Allocator>
NoThrowForwardIt uninitialized_move(InputIt first,
                                    InputIt last,
                                    NoThrowForwardIt dest,
                                    Allocator& alloc) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (mm::details::enable_transfer_without_allocator_v<
                    value_type, InputIt, NoThrowForwardIt,
                    Allocator>) {  // using memmove() for
                                   // trivially copyable types
    return mm::details::uninitialized_copy_trivial_impl(first, last, dest);
  } else {
    return mm::details::uninitialized_move_impl<
        mm::details::emplace_helper_with_allocator>(first, last, dest, alloc);
  }
}

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt uninitialized_move_unckecked(InputIt first,
                                              InputIt last,
                                              NoThrowForwardIt dest) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {  // using memcpy() for
                                                   // trivially copyable types
    return mm::details::uninitialized_copy_unchecked_trivial_impl(first, last,
                                                                  dest);
  } else {
    return mm::details::uninitialized_move_impl<mm::details::emplace_helper>(
        first, last, dest);
  }
}

template <class InputIt, class NoThrowForwardIt, class Allocator>
NoThrowForwardIt uninitialized_move_unckecked(InputIt first,
                                              InputIt last,
                                              NoThrowForwardIt dest,
                                              Allocator& alloc) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (mm::details::enable_transfer_without_allocator_v<
                    value_type, InputIt, NoThrowForwardIt,
                    Allocator>) {  // using memcpy() for
                                   // trivially copyable types
    return mm::details::uninitialized_copy_unchecked_trivial_impl(first, last,
                                                                  dest);
  } else {
    return mm::details::uninitialized_move_impl<
        mm::details::emplace_helper_with_allocator>(first, last, dest, alloc);
  }
}

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt uninitialized_move_n(InputIt first,
                                      size_t count,
                                      NoThrowForwardIt dest) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {  // using memmove() for
                                                   // trivially copyable types
    return mm::details::uninitialized_copy_trivial_impl(first, count, dest);
  } else {
    return mm::details::uninitialized_move_n_impl<mm::details::emplace_helper>(
        first, count, dest);
  }
}

template <class InputIt, class NoThrowForwardIt, class Allocator>
NoThrowForwardIt uninitialized_move_n(InputIt first,
                                      size_t count,
                                      NoThrowForwardIt dest,
                                      Allocator& alloc) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (mm::details::enable_transfer_without_allocator_v<
                    value_type, InputIt, NoThrowForwardIt,
                    Allocator>) {  // using memmove() for
                                   // trivially copyable types
    return mm::details::uninitialized_copy_trivial_impl(first, count, dest);
  } else {
    return mm::details::uninitialized_move_n_impl<
        mm::details::emplace_helper_with_allocator>(first, count, dest, alloc);
  }
}

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt uninitialized_move_n_unchecked(InputIt first,
                                                size_t count,
                                                NoThrowForwardIt dest) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {  // using memcpy() for
                                                   // trivially copyable types
    return mm::details::uninitialized_copy_unchecked_trivial_impl(first, count,
                                                                  dest);
  } else {
    return mm::details::uninitialized_move_n_impl<mm::details::emplace_helper>(
        first, count, dest);
  }
}

template <class InputIt, class NoThrowForwardIt, class Allocator>
NoThrowForwardIt uninitialized_move_n_unchecked(InputIt first,
                                                size_t count,
                                                NoThrowForwardIt dest,
                                                Allocator& alloc) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (mm::details::enable_transfer_without_allocator_v<
                    value_type, InputIt, NoThrowForwardIt,
                    Allocator>) {  // using memcpy() for
                                   // trivially copyable types
    return mm::details::uninitialized_copy_unchecked_trivial_impl(first, count,
                                                                  dest);
  } else {
    return mm::details::uninitialized_move_n_impl<
        mm::details::emplace_helper_with_allocator>(first, count, dest, alloc);
  }
}

namespace mm::details {
template <class Backout>
struct uninitialized_construct_helper : Backout {
 private:
  using MyBase = Backout;

 public:
  using MyBase::MyBase;

  template <class ForwardIt, class Ty>
  void fill_range(ForwardIt last, const Ty& val) {
    while (this->m_dst_cur != last) {
      MyBase::emplace(val);
    }
  }

  template <class Ty>
  void fill_n(size_t count, const Ty& val) {
    for (size_t step = 0; step < count; ++step) {
      MyBase::emplace(val);
    }
  }

  template <class ForwardIt>
  void value_construct_range(ForwardIt last) {
    while (this->m_dst_cur != last) {
      MyBase::value_construct();
    }
  }

  void value_construct_n(size_t count) {
    for (size_t step = 0; step < count; ++step) {
      MyBase::value_construct();
    }
  }

  template <class ForwardIt>
  void default_construct_range(ForwardIt last) {
    while (this->m_dst_cur != last) {
      MyBase::default_construct();
    }
  }

  void default_construct_n(size_t count) {
    for (size_t step = 0; step < count; ++step) {
      MyBase::default_construct();
    }
  }
};

template <class ForwardIt>
using uninitialized_construct =
    uninitialized_construct_helper<uninitialized_backout<ForwardIt> >;

template <class ForwardIt, class Allocator>
using uninitialized_construct_with_allocator = uninitialized_construct_helper<
    uninitialized_backout_with_allocator<ForwardIt, Allocator> >;

template <class Ty>
inline constexpr bool memset_zeroying_is_safe_v =
    is_scalar_v<Ty> && !is_member_pointer_v<Ty> && !is_volatile_v<Ty>;

template <class Ty>
bool all_bits_are_zeroes(const Ty& value) {
  static_assert(is_scalar_v<Ty> && !is_member_pointer_v<Ty>);
  constexpr Ty reference{};
  return memcmp(addressof(value), addressof(reference), sizeof(Ty)) == 0;
}

template <template <class... Types> class FillHelper,
          class ForwardIt,
          class Ty,
          class... Allocators>
ForwardIt uninitialized_fill_impl(ForwardIt first,
                                  ForwardIt last,
                                  const Ty& val,
                                  Allocators&... allocs) {
  FillHelper<ForwardIt, Allocators...> uconh(first, allocs...);
  uconh.fill_range(last, val);
  return uconh.release();
}

template <template <class... Types> class FillHelper,
          class ForwardIt,
          class Ty,
          class... Allocators>
ForwardIt uninitialized_fill_n_impl(ForwardIt first,
                                    size_t count,
                                    const Ty& val,
                                    Allocators&... allocs) {
  FillHelper<ForwardIt, Allocators...> uconh(first, allocs...);
  uconh.fill_n(count, val);
  return uconh.release();
}

template <class Ty, class OutputIt, class FillerTy>
constexpr bool is_memset_safe(const FillerTy& value) {
  if constexpr (!is_pointer_v<OutputIt>) {
    return false;
  } else {
    if constexpr (memset_is_safe_v<Ty>) {
      return true;
    } else {
      if constexpr (!memset_zeroying_is_safe_v<Ty>) {
        return false;
      } else {
        return all_bits_are_zeroes(value);
      }
    }
  }
}

template <class Ty, class OutputIt, class Allocator, class FillerTy>
constexpr bool is_memset_safe_with_allocator(const FillerTy& value) {
  if constexpr (!is_pointer_v<OutputIt>) {
    if constexpr (has_construct_v<
                      Allocator, typename iterator_traits<OutputIt>::pointer>) {
      return false;
    } else {
      if constexpr (memset_is_safe_v<Ty>) {
        return true;
      } else {
        if constexpr (!memset_zeroying_is_safe_v<Ty>) {
          return false;
        } else {
          return all_bits_are_zeroes(value);
        }
      }
    }
  }
}

template <class Ty, class OutputIt, class Allocator>
inline constexpr bool enable_wmemset_with_allocator_v =
    wmemset_is_safe_v<Ty>&& is_pointer_v<OutputIt> &&
    !has_construct_v<Allocator, typename iterator_traits<OutputIt>::pointer>;
}  // namespace mm::details

template <class ForwardIt, class Ty>
ForwardIt uninitialized_fill(ForwardIt first, ForwardIt last, const Ty& value) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;

  if constexpr (mm::details::wmemset_is_safe_v<value_type> &&
                is_pointer_v<ForwardIt>) {
    return wmemset(first, value, static_cast<size_t>(last - first));
  } else {
    if (mm::details::is_memset_safe<value_type, ForwardIt>(value)) {
      return static_cast<ForwardIt>(
          memset(first, reinterpret_cast<int>(value),
                 mm::details::distance_in_bytes(first, last)));
    }
    return mm::details::uninitialized_fill_impl<
        mm::details::uninitialized_construct>(first, last, value);
  }
}

template <class ForwardIt, class Ty, class Allocator>
ForwardIt uninitialized_fill(ForwardIt first,
                             ForwardIt last,
                             const Ty& value,
                             Allocator& alloc) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;

  if constexpr (mm::details::enable_wmemset_with_allocator_v<
                    value_type, ForwardIt, Allocator>) {
    return wmemset(first, value, static_cast<size_t>(last - first));
  } else {
    if (mm::details::is_memset_safe_with_allocator<value_type, ForwardIt,
                                                   Allocator>(value)) {
      return static_cast<ForwardIt>(
          memset(first, static_cast<int>(value),
                 mm::details::distance_in_bytes(first, last)));
    }
    return mm::details::uninitialized_fill_impl<
        mm::details::uninitialized_construct_with_allocator>(first, last,
                                                             value);
  }
}

template <class ForwardIt, class Ty>
ForwardIt uninitialized_fill_n(ForwardIt first, size_t count, const Ty& value) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;

  if constexpr (mm::details::wmemset_is_safe_v<value_type> &&
                is_pointer_v<ForwardIt>) {
    return wmemset(first, value, count);
  } else {
    if (mm::details::is_memset_safe<value_type, ForwardIt>(value)) {
      return static_cast<ForwardIt>(
          memset(first, static_cast<int>(value), count * sizeof(value_type)));
    }
    return mm::details::uninitialized_fill_n_impl<
        mm::details::uninitialized_construct>(first, count, value);
  }
}

template <class ForwardIt, class Ty, class Allocator>
ForwardIt uninitialized_fill_n(ForwardIt first,
                               size_t count,
                               const Ty& value,
                               Allocator& alloc) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;

  if constexpr (mm::details::enable_wmemset_with_allocator_v<
                    value_type, ForwardIt, Allocator>) {
    return wmemset(first, value, count);
  } else {
    if (mm::details::is_memset_safe_with_allocator<value_type, ForwardIt,
                                                   Allocator>(value)) {
      return static_cast<ForwardIt>(
          memset(first, static_cast<int>(value), count * sizeof(value_type)));
    }
    return mm::details::uninitialized_fill_n_impl<
        mm::details::uninitialized_construct_with_allocator>(first, count,
                                                             value, alloc);
  }
}

namespace mm::details {
template <template <class... Types> class ConstructHelper,
          class ForwardIt,
          class... Allocators>
ForwardIt uninitialized_value_construct_impl(ForwardIt first,
                                             ForwardIt last,
                                             Allocators&... allocs) {
  ConstructHelper<ForwardIt, Allocators...> uconh(first, allocs...);
  uconh.value_construct_range(last);
  return uconh.release();
}

template <template <class... Types> class ConstructHelper,
          class ForwardIt,
          class... Allocators>
ForwardIt uninitialized_value_construct_n_impl(ForwardIt first,
                                               size_t count,
                                               Allocators&... allocs) {
  ConstructHelper<ForwardIt, Allocators...> uconh(first, allocs...);
  uconh.value_construct_n(count);
  return uconh.release();
}

template <template <class... Types> class ConstructHelper,
          class ForwardIt,
          class... Allocators>
ForwardIt uninitialized_default_construct_impl(ForwardIt first,
                                               ForwardIt last,
                                               Allocators&... allocs) {
  ConstructHelper<ForwardIt, Allocators...> uconh(first, allocs...);
  uconh.default_construct_range(last);
  return uconh.release();
}

template <template <class... Types> class ConstructHelper,
          class ForwardIt,
          class... Allocators>
ForwardIt uninitialized_default_construct_n_impl(ForwardIt first,
                                                 size_t count,
                                                 Allocators&... allocs) {
  ConstructHelper<ForwardIt, Allocators...> uconh(first, allocs...);
  uconh.default_construct_n(count);
  return uconh.release();
}

template <class Ty, class OutputIt, class Allocator>
inline constexpr bool enable_trivial_construct_with_allocator_v =
    is_trivially_default_constructible_v<Ty>&& is_pointer_v<OutputIt> &&
    !has_construct_v<Allocator, typename iterator_traits<OutputIt>::pointer>;
}  // namespace mm::details

template <class ForwardIt>
ForwardIt uninitialized_value_construct(ForwardIt first, ForwardIt last) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;

  if constexpr (is_trivially_default_constructible_v<value_type> &&
                is_pointer_v<ForwardIt>) {
    return static_cast<ForwardIt>(
        memset(first, 0, mm::details::distance_in_bytes(first, last)));
  } else {
    return mm::details::uninitialized_value_construct_impl<
        mm::details::uninitialized_construct>(first, last);
  }
}

template <class ForwardIt, class Allocator>
ForwardIt uninitialized_value_construct(ForwardIt first,
                                        ForwardIt last,
                                        Allocator& alloc) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;

  if constexpr (is_trivially_default_constructible_v<value_type> &&
                is_pointer_v<ForwardIt>) {
    return static_cast<ForwardIt>(
        memset(first, 0, mm::details::distance_in_bytes(first, last)));
  } else {
    return mm::details::uninitialized_value_construct_impl<
        mm::details::uninitialized_construct_with_allocator>(first, last,
                                                             alloc);
  }
}

template <class ForwardIt>
ForwardIt uninitialized_value_construct_n(ForwardIt first, size_t count) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;

  if constexpr (is_trivially_default_constructible_v<value_type> &&
                is_pointer_v<ForwardIt>) {
    return static_cast<ForwardIt>(memset(first, 0, count * sizeof(value_type)));
  } else {
    return mm::details::uninitialized_value_construct_n_impl<
        mm::details::uninitialized_construct>(first, count);
  }
}

template <class ForwardIt, class Allocator>
ForwardIt uninitialized_value_construct_n(ForwardIt first,
                                          size_t count,
                                          Allocator& alloc) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;

  if constexpr (mm::details::enable_trivial_construct_with_allocator_v<
                    value_type, ForwardIt, Allocator>) {
    return static_cast<ForwardIt>(memset(first, 0, count * sizeof(value_type)));
  } else {
    return mm::details::uninitialized_value_construct_n_impl<
        mm::details::uninitialized_construct_with_allocator>(first, count,
                                                             alloc);
  }
}

template <class ForwardIt>
ForwardIt uninitialized_default_construct(ForwardIt first, ForwardIt last) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;

  if constexpr (is_trivially_default_constructible_v<value_type> &&
                is_pointer_v<ForwardIt>) {
    return static_cast<ForwardIt>(
        memset(first, 0, mm::details::distance_in_bytes(first, last)));
  } else {
    return mm::details::uninitialized_default_construct_impl<
        mm::details::uninitialized_construct>(first, last);
  }
}

template <class ForwardIt, class Allocator>
ForwardIt uninitialized_default_construct(ForwardIt first,
                                          ForwardIt last,
                                          Allocator& alloc) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;

  if constexpr (mm::details::enable_trivial_construct_with_allocator_v<
                    value_type, ForwardIt, Allocator>) {
    return static_cast<ForwardIt>(
        memset(first, 0, mm::details::distance_in_bytes(first, last)));
  } else {
    return mm::details::uninitialized_default_construct_impl<
        mm::details::uninitialized_construct_with_allocator>(first, last,
                                                             alloc);
  }
}

template <class ForwardIt>
ForwardIt uninitialized_default_construct_n(ForwardIt first, size_t count) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;

  if constexpr (is_trivially_default_constructible_v<value_type> &&
                is_pointer_v<ForwardIt>) {
    return static_cast<ForwardIt>(memset(first, 0, count * sizeof(value_type)));
  } else {
    return mm::details::uninitialized_default_construct_n_impl<
        mm::details::uninitialized_construct>(first, count);
  }
}

template <class ForwardIt, class Allocator>
ForwardIt uninitialized_default_construct_n(ForwardIt first,
                                            size_t count,
                                            Allocator& alloc) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;

  if constexpr (mm::details::enable_trivial_construct_with_allocator_v<
                    value_type, ForwardIt, Allocator>) {
    return static_cast<ForwardIt>(memset(first, 0, count * sizeof(value_type)));
  } else {
    return mm::details::uninitialized_default_construct_n_impl<
        mm::details::uninitialized_construct_with_allocator>(first, count,
                                                             alloc);
  }
}
#endif
}  // namespace ktl
