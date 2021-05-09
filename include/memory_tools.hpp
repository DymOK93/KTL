#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <memory>
#include <utility>
// there is no std::construct_at in C++17
using std::destroy;
using std::destroy_at;
using std::destroy_n;
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
#include <heap.h>
#include <iterator.hpp>
#include <memory_type_traits.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

namespace ktl {
template <class Ty, class... Types>
constexpr Ty* construct_at(Ty* place, Types&&... args) noexcept(
    is_nothrow_constructible_v<Ty, Types...>) {
  return new (const_cast<void*>(static_cast<const volatile void*>(place)))
      Ty(forward<Types>(args)...);
}

template <class Ty>
constexpr Ty* default_construct_at(Ty* place) noexcept(
    is_nothrow_default_constructible_v<Ty>) {
  return new (const_cast<void*>(static_cast<const volatile void*>(place))) Ty;
}

template <class Ty>
constexpr void destroy_at(
    Ty* object_ptr) noexcept {  // CRASH if d-tor isn't noexcept
  object_ptr->~Ty();
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

namespace mm::details {
template <class InputTy, class OutputTy>
OutputTy* uninitialized_copy_trivial_impl(InputTy* first,
                                          InputTy* last,
                                          OutputTy* dest) {
  return static_cast<OutputTy*>(
      memmove(dest, first, (last - first) * sizeof(InputTy)));
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
      memcpy(dest, first, (last - first) * sizeof(InputTy)));
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
  ~uninitialized_backout_base() { destroy(m_dst_first, m_dst_cur); }

  ForwardIt release() {
    m_dst_first = m_dst_cur;
    return m_dst_cur;
  }

 protected:
  template <class... Types>
  decltype(auto) emplace(Types&&... args) {
    auto& place{addressof(*m_dst_cur++)};
    construct_at(addressof(place), forward<Types>(args)...);
    m_dst_cur = next(m_dst_cur);
    return place;
  }

 protected:
  ForwardIt m_dst_first;
  ForwardIt m_dst_cur;
};

template <class ForwardIt>
class uninitialized_emplace_helper
    : public uninitialized_backout_base<ForwardIt> {
 public:
  using MyBase = uninitialized_backout_base<ForwardIt>;

 public:
  using MyBase::MyBase;

  template <class InputIt>
  void copy_range(InputIt first, InputIt last) {
    for (; first != last; first = next(first)) {
      MyBase::emplace(as_const(*first));  // InputIt may be move iterator
    }
  }

  template <class InputIt>
  void copy_n(InputIt first, size_t count) {
    for (size_t step = 0; step != count; first = next(first), ++step) {
      MyBase::emplace(as_const(*first));  // InputIt may be move iterator
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

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt uninitialized_copy_non_trivial_impl(InputIt first,
                                                     InputIt last,
                                                     NoThrowForwardIt dest) {
  uninitialized_emplace_helper uemph(dest);
  uemph.copy_range(first, last);
  return uemph.release();
}

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt uninitialized_copy_unchecked_non_trivial_impl(
    InputIt first,
    InputIt last,
    NoThrowForwardIt dest) {
  uninitialized_emplace_helper uemph(dest);
  uemph.copy_range(first, last);
  return uemph.release();
}

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt uninitialized_copy_n_non_trivial_impl(InputIt first,
                                                       size_t count,
                                                       NoThrowForwardIt dest) {
  uninitialized_emplace_helper uemph(dest);
  uemph.copy_n(first, count);
  return uemph.release();
}

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt uninitialized_copy_n_unchecked_non_trivial_impl(
    InputIt first,
    size_t count,
    NoThrowForwardIt dest) {
  uninitialized_emplace_helper uemph(dest);
  uemph.copy_n(first, count);
  return uemph.release();
}

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt uninitialized_move_impl(InputIt first,
                                         InputIt last,
                                         NoThrowForwardIt dest) {
  uninitialized_emplace_helper uemph(dest);
  uemph.move_range(first, last);
  return uemph.release();
}

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt uninitialized_move_unchecked_non_trivial_impl(
    InputIt first,
    size_t count,
    NoThrowForwardIt dest) {
  uninitialized_emplace_helper uemph(dest);
  uemph.move_n(first, count);
  return uemph.release();
}

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt uninitialized_move_n_non_trivial_impl(InputIt first,
                                                       size_t count,
                                                       NoThrowForwardIt dest) {
  uninitialized_emplace_helper uemph(dest);
  uemph.move_n(first, count);
  return uemph.release();
}

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt uninitialized_move_n_unchecked_non_trivial_impl(
    InputIt first,
    size_t count,
    NoThrowForwardIt dest) {
  uninitialized_emplace_helper uemph(dest);
  uemph.move_n(first, count);
  return uemph.release();
}
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
    return mm::details::uninitialized_copy_non_trivial_impl(first, last, dest);
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
    return mm::details::uninitialized_copy_unchecked_non_trivial_impl(
        first, last, dest);
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
    return mm::details::uninitialized_copy_non_trivial_impl(first, count, dest);
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
    return mm::details::uninitialized_copy_n_unchecked_non_trivial_impl(
        first, count, dest);
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
    return mm::details::uninitialized_move_impl(first, last, dest);
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
    return mm::details::uninitialized_move_unchecked_non_trivial_impl(
        first, last, dest);
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
    return mm::details::uninitialized_move_impl(first, count, dest);
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
    return mm::details::uninitialized_move_n_unchecked_non_trivial_impl(
        first, count, dest);
  }
}

namespace mm::details {
template <class ForwardIt>
class uninitialized_construct_base
    : public uninitialized_backout_base<ForwardIt> {
 private:
  using MyBase = uninitialized_backout_base<ForwardIt>;

 public:
  using MyBase::MyBase;

 protected:
  decltype(auto) value_construct() { return MyBase::emplace(); }
  decltype(auto) default_construct() {
    auto* place{this->m_dst_cur++};
    default_construct_at(place);
    return *place;
  }
};

template <class ForwardIt>
class uninitialized_construct_helper
    : public uninitialized_construct_base<ForwardIt> {
 private:
  using MyBase = uninitialized_construct_base<ForwardIt>;

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
uninitialized_construct_helper(ForwardIt)
    -> uninitialized_construct_helper<ForwardIt>;
}  // namespace mm::details

template <class ForwardIt, class Ty>
ForwardIt uninitialized_fill(ForwardIt first, ForwardIt last, const Ty& val) {
  if constexpr (mm::details::memset_is_safe_v<Ty> && is_pointer_v<ForwardIt>) {
    return static_cast<ForwardIt>(
        memset(first, static_cast<int>(val), distance(first, last)));
  } else {
    mm::details::uninitialized_construct_helper uconh(first);
    uconh.fill_range(last, val);
    return uconh.release();
  }
}

template <class ForwardIt, class Ty>
ForwardIt uninitialized_fill_n(ForwardIt first, size_t count, const Ty& val) {
  if constexpr (mm::details::memset_is_safe_v<Ty> && is_pointer_v<ForwardIt>) {
    return static_cast<ForwardIt>(memset(first, static_cast<int>(val), count));
  } else {
    mm::details::uninitialized_construct_helper uconh(first);
    uconh.fill_n(count, val);
    return uconh.release();
  }
}

template <class ForwardIt>
ForwardIt uninitialized_value_construct(ForwardIt first, ForwardIt last) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;
  if constexpr (is_trivially_default_constructible_v<value_type> &&
                is_pointer_v<ForwardIt>) {
    size_t bytes_count{distance(first, last) * sizeof(value_type)};
    return static_cast<ForwardIt>(memset(first, 0, bytes_count));
  } else {
    mm::details::uninitialized_construct_helper uconh(first);
    uconh.value_construct_range(last);
    return uconh.release();
  }
}

template <class ForwardIt>
ForwardIt uninitialized_value_construct_n(ForwardIt first, size_t count) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;
  if constexpr (is_trivially_default_constructible_v<value_type> &&
                is_pointer_v<ForwardIt>) {
    return static_cast<ForwardIt>(memset(first, 0, count * sizeof(value_type)));
  } else {
    mm::details::uninitialized_construct_helper uconh(first);
    uconh.value_construct_n(count);
    return uconh.release();
  }
}

template <class ForwardIt>
ForwardIt uninitialized_default_construct(ForwardIt first, ForwardIt last) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;
  if constexpr (is_trivially_default_constructible_v<value_type> &&
                is_pointer_v<ForwardIt>) {
    size_t bytes_count{distance(first, last) * sizeof(value_type)};
    return static_cast<ForwardIt>(memset(first, 0, bytes_count));
  } else {
    mm::details::uninitialized_construct_helper uconh(first);
    uconh.default_construct_range(last);
    return uconh.release();
  }
}

template <class ForwardIt>
ForwardIt uninitialized_default_construct_n(ForwardIt first, size_t count) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;
  if constexpr (is_trivially_default_constructible_v<value_type> &&
                is_pointer_v<ForwardIt>) {
    return static_cast<ForwardIt>(memset(first, 0, count * sizeof(value_type)));
  } else {
    mm::details::uninitialized_construct_helper uconh(first);
    uconh.default_construct_n(count);
    return uconh.release();
  }
}
}  // namespace ktl
#endif
