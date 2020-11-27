#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <memory>
#include <utility>
using std::uninitialized_copy;
using std::uninitialized_copy_n;
#else
#include <heap.h>
#include <utility.hpp>
#include <type_traits.hpp>
#include <memory_type_traits.hpp>
#include <iterator.hpp>

namespace winapi::kernel {
template <class Ty, class... Types>
constexpr Ty* construct_at(Ty* place, Types&&... args) noexcept(
    is_nothrow_constructible_v<Ty, Types...>) {
  return new (const_cast<void*>(static_cast<const volatile void*>(place)))
      Ty(forward<Types>(args)...);
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
  using value_type = mm::details::unfancy_pointer_t<InputIt>;
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
  using value_type = mm::details::unfancy_pointer_t<InputIt>;
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
class uninitialized_emplace_helper {
 public:
  uninitialized_emplace_helper(ForwardIt dst)
      : m_dst_first{dst}, m_dst_cur{dst} {}

#ifndef KTL_NO_EXCEPTIONS
  template <class... Types>
  decltype(auto) emplace(Types&&... args) {
    auto& place{*m_dst_cur};
    __try {
      construct_at(addressof(place), forward<Types>(args)...);
      m_dst_cur = next(m_dst_cur);
      return place;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      destroy(m_dst_cur, m_dst_first);
      ExRaiseStatus(GetExceptionCode());  // Re-raise exception
    }
  }
#else
  template <class... Types>
  void emplace_unchecked(Types&&... args) {
    auto& place{*m_dst_cur};
    construct_at(addressof(place), forward<Types>(args)...);
  }

  template <class Verifier, class... Types>
  bool emplace(Verifier&& vf, Types&&... args) {
    auto& place{*m_dst_cur};
    construct_at(addressof(place), forward<Types>(args)...);
    if (!vf(place)) {  // if считается explicit bool cast'ом
      destroy(m_dst_cur, m_dst_first);
      return false;
    }
    return true;
  }
#endif

  ForwardIt get_position() const { return m_dst_cur; }

 private:
  ForwardIt m_dst_first;
  ForwardIt m_dst_cur;
};

#ifndef KTL_NO_EXCEPTIONS
template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt
#else
template <class Verifier, class InputIt, class NoThrowForwardIt>
bool
#endif
uninitialized_copy_non_trivial_impl(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    InputIt first,
    InputIt last,
    NoThrowForwardIt dest) {
  uninitialized_emplace_helper uemph(dest);

#ifndef KTL_NO_EXCEPTIONS
  for (first; first != last; first = next(first)) {
    uemph.emplace(*first);
  }
  return uemph.get_position();
#else
  bool successfully_copied{true};
  for (first; first != last && successfully_copied; first = next(first)) {
    successfully_copied = uemph.emplace(vf, *first);
  }
  return successfully_copied;
#endif
}

#ifndef KTL_NO_EXCEPTIONS
template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt
#else
template <class Verifier, class InputIt, class NoThrowForwardIt>
bool
#endif
uninitialized_copy_non_trivial_impl(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    InputIt first,
    size_t count,
    NoThrowForwardIt dest) {
  uninitialized_emplace_helper uemph(dest);
#ifndef KTL_NO_EXCEPTIONS
  for (size_t step = 0; step < count; first = next(first), ++step) {
    uemph.emplace(*first);
  }
  return uemph.get_position();
#else
  bool successfully_copied{true};
  for (size_t step = 0; step < count; first = next(first), ++step) {
    successfully_copied = uemph.emplace(vf, *first);
  }
  return successfully_copied;
#endif
}

#ifndef KTL_NO_EXCEPTIONS
template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt
#else
template <class Verifier, class InputIt, class NoThrowForwardIt>
bool
#endif
uninitialized_move_impl(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    InputIt first,
    InputIt last,
    NoThrowForwardIt dest) {
  uninitialized_emplace_helper uemph(dest);
#ifndef KTL_NO_EXCEPTIONS
  for (first; first != last; first = next(first)) {
    uemph.emplace(move(*first));
  }
  return uemph.get_position();
#else
  bool successfully_moved{true};
  for (first; first != last && successfully_copied; first = next(first)) {
    successfully_moved = uemph.emplace(vf, move(*first));
  }
  return successfully_moved;
#endif
}

#ifndef KTL_NO_EXCEPTIONS
template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt
#else
template <class Verifier, class InputIt, class NoThrowForwardIt>
bool
#endif
uninitialized_move_impl(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    InputIt first,
    size_t count,
    NoThrowForwardIt dest) {
  uninitialized_emplace_helper uemph(dest);
#ifndef KTL_NO_EXCEPTIONS
  for (size_t step = 0; step < count; first = next(first), ++step) {
    uemph.emplace(move(*first));
  }
  return uemph.get_position();
#else
  bool successfully_moved{true};
  for (size_t step = 0; step < count; first = next(first), ++step) {
    successfully_moved = uemph.emplace(vf, move(*first));
  }
  return successfully_moved;
#endif
}
}  // namespace mm::details

#ifndef KTL_NO_EXCEPTIONS
template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt
#else
template <class Verifier, class InputIt, class NoThrowForwardIt>
#endif  // using memmove() for trivially copyable types
uninitialized_copy(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    InputIt first,
    InputIt last,
    NoThrowForwardIt dest) {
  using value_type = mm::details::unfancy_pointer_t<InputIt>;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {
    return mm::details::uninitialized_copy_trivial_impl(first, last,
                                                        dest);  // always true
  } else {
#ifndef KTL_NO_EXCEPTIONS
    return mm::details::uninitialized_copy_non_trivial_impl(first, last, dest);
#else
    return mm::details::uninitialized_copy_non_trivial_impl(vf, first, last,
                                                            dest);
#endif
  }
}

#ifndef KTL_NO_EXCEPTIONS
template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt
#else
template <class Verifier, class InputIt, class NoThrowForwardIt>
bool
#endif  // using memcpy() for trivially copyable types
uninitialized_copy_unchecked(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    InputIt first,
    InputIt last,
    NoThrowForwardIt dest) {
  using value_type = mm::details::unfancy_pointer_t<InputIt>;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {
    return mm::details::uninitialized_copy_unchecked_trivial_impl(
        first, last,  // always true
        dest);
  } else {
#ifndef KTL_NO_EXCEPTIONS
    return mm::details::uninitialized_copy_non_trivial_impl(first, last, dest);
#else
    return mm::details::uninitialized_copy_non_trivial_impl(vf, first, last,
                                                            dest);
#endif
  }
}

#ifndef KTL_NO_EXCEPTIONS
template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt
#else
template < class Verifier, class InputIt, class NoThrowForwardIt>
bool
#endif  // using memmove() for trivially copyable
        // types
uninitialized_copy_n(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    InputIt first,
    size_t count,
    NoThrowForwardIt dest) {
  using value_type = mm::details::unfancy_pointer_t<InputIt>;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {
    return mm::details::uninitialized_copy_trivial_impl(first, count,
                                                        dest);  // always true
  } else {
#ifndef KTL_NO_EXCEPTIONS
    return mm::details::uninitialized_copy_non_trivial_impl(first, count, dest);
#else
    return mm::details::uninitialized_copy_non_trivial_impl(first, count, dest);
#endif
  }
}

#ifndef KTL_NO_EXCEPTIONS
template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt
#else
template <class Verifier, class InputIt, class NoThrowForwardIt>
bool  // using memcpy() for trivially copyable types
#endif
uninitialized_copy_n_unchecked(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    InputIt first,
    size_t count,
    NoThrowForwardIt dest) {
  using value_type = mm::details::unfancy_pointer_t<InputIt>;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {
    return mm::details::uninitialized_copy_unchecked_trivial_impl(first, count,
                                                                  dest);
  } else {
#ifndef KTL_NO_EXCEPTIONS
    return mm::details::uninitialized_copy_non_trivial_impl(first, count, dest);
#else
    return mm::details::uninitialized_copy_non_trivial_impl(vf, first, count,
                                                            dest);
#endif
  }
}

#ifndef KTL_NO_EXCEPTIONS
template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt
#else
template <class Verifier, class InputIt, class NoThrowForwardIt>
bool
#endif  // using memmove() for trivially copyable types
uninitialized_move(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    InputIt first,
    InputIt last,
    NoThrowForwardIt dest) {
  using value_type = mm::details::unfancy_pointer_t<InputIt>;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {
    return mm::details::uninitialized_copy_trivial_impl(first, last, dest);
  } else {
#ifndef KTL_NO_EXCEPTIONS
    return mm::details::uninitialized_move_impl(first, last, dest);
#else
    return mm::details::uninitialized_move_impl(vf, first, last, dest);
#endif
  }
}

#ifndef KTL_NO_EXCEPTIONS
template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt
#else
template <class Verifier, class InputIt, class NoThrowForwardIt>
bool
#endif  // using memmove() for trivially copyable types
uninitialized_move_n(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    InputIt first,
    size_t count,
    NoThrowForwardIt dest) {
  using value_type = mm::details::unfancy_pointer_t<InputIt>;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {
    return mm::details::uninitialized_copy_trivial_impl(first, count, dest);
  } else {
#ifndef KTL_NO_EXCEPTIONS
    return mm::details::uninitialized_move_impl(first, count, dest);
#else
    return mm::details::uninitialized_move_impl(vf, first, count, dest);
#endif
  }
}
}  // namespace winapi::kernel
#endif
