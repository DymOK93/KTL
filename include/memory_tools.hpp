#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <memory>
#include <utility>
using std::uninitialized_copy;
using std::uninitialized_copy_n;
using std::uninitialized_move;
using std::uninitialized_move_n;
using std::uninitialized_fill;
using std::uninitialized_fill_n;

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
class uninitialized_backout_base {
 public:
  uninitialized_backout_base(ForwardIt dst)
      : : m_dst_first{dst}, m_dst_cur{dst} {}
  ~uninitialized_backout_base() { destroy(m_dst_first, m_dst_cur); }

 protected:
  ForwardIt release() {
    m_dst_cur = m_dst_first;
    return m_dst_cur;
  }

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

template <class Verifier, class ForwardIt>
class uninitialized_verified_emplace_helper
    : public uninitialized_backout_base<ForwardIt> {
 public:
  using MyBase = uninitialized_backout_base<ForwardIt>;

 public:
  uninitialized_verified_emplace_helper(Verifier& vf, ForwardIt dest)
      : MyBase(dest), m_vf{vf}, m_success{false} {}

  uninitialized_verified_emplace_helper(
      const uninitialized_verified_emplace_helper&) = delete;
  uninitialized_verified_emplace_helper& operator=(
      const uninitialized_verified_emplace_helper&) = delete;

  template <class InputIt>
  void copy_range(InputIt first, InputIt last) {
    bool success{true};
    for (; first != last && success; first = next(first)) {
      success = bool_cast(m_vf(
          MyBase::emplace(as_const(*first))));  // InputIt may be move iterator
    }
  }

  template <class InputIt>
  void copy_n(InputIt first, size_t count) {
    bool success{true};
    for (size_t step = 0; step != count && success;
         first = next(first), ++step) {
      success = bool_cast(m_vf(
          MyBase::emplace(as_const(*first))));  // InputIt may be move iterator
    }
  }

  template <class InputIt>
  void move_range(InputIt first, InputIt last) {
    bool success{true};
    for (; first != last && success; first = next(first)) {
      success = bool_cast(
          m_vf(MyBase::emplace(move(*first))));  // InputIt may be move iterator
    }
  }

  template <class InputIt>
  void move_n(InputIt first, size_t count) {
    bool success{true};
    for (size_t step = 0; step != count && success;
         first = next(first), ++step) {
      success = bool_cast(
          m_vf(MyBase::emplace(move(*first))));  // InputIt may be move iterator
    }
  }

  bool release() {
    if (m_success) {
      MyBase::release();
      return true;
    }
    return false;  // range will be destroyed
  }

 private:
  Verifier& m_vf;
  bool m_success;
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
#ifndef KTL_NO_EXCEPTIONS
  uninitialized_emplace_helper uemph(dest);
#else
  uninitialized_verified_emplace_helper uemph(vf, dest);
#endif
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

#ifndef KTL_NO_EXCEPTIONS
template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt
#else
template <class Verifier, class InputIt, class NoThrowForwardIt>
bool
#endif
uninitialized_copy_n_non_trivial_impl(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    InputIt first,
    size_t count,
    NoThrowForwardIt dest) {
#ifndef KTL_NO_EXCEPTIONS
  uninitialized_emplace_helper uemph(dest);
#else
  uninitialized_verified_emplace_helper uemph(dest);
#endif
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
#ifndef KTL_NO_EXCEPTIONS
  uninitialized_emplace_helper uemph(dest);
#else
  uninitialized_verified_emplace_helper uemph(vf, dest);
#endif
  uemph.move_range(first, last);
  return uemph.release();
}

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt uninitialized_move_unchecked_non_trivial_impl(
    InputIt first,
    size_t count,
    NoThrowForwardIt dest) {
  uninitialized_emplace_helper uemph(dest);
  uemph.move_range(first, last);
  return uemph.release();
}

#ifndef KTL_NO_EXCEPTIONS
template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt
#else
template <class Verifier, class InputIt, class NoThrowForwardIt>
bool
#endif
uninitialized_move_n_non_trivial_impl(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    InputIt first,
    size_t count,
    NoThrowForwardIt dest) {
#ifndef KTL_NO_EXCEPTIONS
  uninitialized_emplace_helper uemph(dest);
#else
  uninitialized_verified_emplace_helper uemph(vf, dest);
#endif
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

#ifndef KTL_NO_EXCEPTIONS
template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt
#else
template <class Verifier, class InputIt, class NoThrowForwardIt>
bool
#endif  // using memmove() for trivially copyable types
uninitialized_copy(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    InputIt first,
    InputIt last,
    NoThrowForwardIt dest) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {
#ifdef KTL_NO_EXCEPTIONS
    (void)vf;
#endif
    return bool_cast(
        mm::details::uninitialized_copy_trivial_impl(first, last,
                                                     dest));  // always true
  } else {
#ifndef KTL_NO_EXCEPTIONS
    return mm::details::uninitialized_copy_non_trivial_impl(first, last, dest);
#else
    return mm::details::uninitialized_copy_non_trivial_impl(vf, first, last,
                                                            dest);
#endif
  }
}

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt  // using memcpy() for trivially copyable types
uninitialized_copy_unchecked(InputIt first,
                             InputIt last,
                             NoThrowForwardIt dest) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {
    return mm::details::uninitialized_copy_unchecked_trivial_impl(
        first, last,  // always true
        dest);
  } else {
    return mm::details::uninitialized_copy_unchecked_non_trivial_impl(
        first, last, dest);
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
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {
#ifdef KTL_NO_EXCEPTIONS
    (void)vf;
#endif
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

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt  // using memcpy() for trivially copyable types
uninitialized_copy_n_unchecked(InputIt first,
                               size_t count,
                               NoThrowForwardIt dest) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {
    return mm::details::uninitialized_copy_unchecked_trivial_impl(first, count,
                                                                  dest);
  } else {
    return mm::details::uninitialized_copy_n_unchecked_non_trivial_impl(
        first, count, dest);
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
  using value_type = typename iterator_traits<InputIt>::value_type;
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

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt  // using memcpy() for trivially copyable types
uninitialized_move_unckecked(InputIt first,
                             InputIt last,
                             NoThrowForwardIt dest) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {
    return mm::details::uninitialized_copy_unchecked_trivial_impl(first, last,
                                                                  dest);
  } else {
    return mm::details::uninitialized_move_unchecked_non_trivial_impl(
        first, last, dest);
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
  using value_type = typename iterator_traits<InputIt>::value_type;
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

template <class InputIt, class NoThrowForwardIt>
NoThrowForwardIt  // using memcpy() for trivially copyable types
uninitialized_move_n(InputIt first, size_t count, NoThrowForwardIt dest) {
  using value_type = typename iterator_traits<InputIt>::value_type;
  if constexpr (is_trivially_copyable_v<value_type> && is_pointer_v<InputIt> &&
                is_pointer_v<NoThrowForwardIt>) {
    return mm::details::uninitialized_copy_unchecked_trivial_impl(first, count,
                                                                  dest);
  } else {
    return mm::details::uninitialized_move_n_unchecked_non_trivial_impl(
        vf, first, count, dest);
  }
}

namespace mm::details {
template <class ForwardIt>
class uninitialized_construct_base
    : public uninitialized_backout_base<ForwardIt> {
 public:
  using MyBase = uninitialized_backout_base<ForwardIt>;

 public:
  using MyBase::MyBase;

 protected:
  decltype(auto) value_construct() { return MyBase::emplace(); }
  decltype(auto) default_construct() {
    auto& place{*m_dst_cur++};
    default_construct_at(addressof(place));
    return place;
  }
};

template <class ForwardIt>
class uninitialized_construct_helper
    : public uninitialized_construct_base<ForwardIt> {
 public:
  using MyBase = uninitialized_construct_base<ForwardIt>;

 public:
  using MyBase::MyBase;

  template <class ForwardIt, class Ty>
  void fill_range(ForwardIt last, const Ty& val) {
    while (m_dst_cur != last) {
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
    while (m_dst_cur != last) {
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
    while (m_dst_cur != last) {
      MyBase::default_construct();
    }
  }

  void value_construct_n(size_t count) {
    for (size_t step = 0; step < count; ++step) {
      MyBase::default_construct();
    }
  }
};

template <class Verifier, class ForwardIt>
class uninitialized_verified_construct_helper
    : public uninitialized_backout_base<ForwardIt> {
 public:
  using MyBase = uninitialized_backout_base<ForwardIt>;

 public:
  uninitialized_verified_construct_helper(Verifier& vf, ForwardIt dest)
      : MyBase(dest), m_vf{vf}, m_success{false} {}

  template <class ForwardIt, class Ty>
  void fill_range(ForwardIt last, const Ty& val) {
    while (m_dst_cur != last && m_success) {
      m_success = bool_cast(vf(MyBase::emplace(val)));
    }
  }

  template <class Ty>
  void fill_n(size_t count, const Ty& val) {
    for (size_t step = 0; step < count && m_success; ++step) {
      m_success = bool_cast(vf(MyBase::emplace(val)));
    }
  }

  template <class ForwardIt>
  void value_construct_range(ForwardIt last) {
    while (m_dst_cur != last && m_success) {
      m_success = bool_cast(vf(MyBase::value_construct()));
    }
  }

  void value_construct_n(size_t count) {
    for (size_t step = 0; step < count && m_success; ++step) {
      m_success = bool_cast(vf(MyBase::value_construct()));
    }
  }

  template <class ForwardIt>
  void default_construct_range(ForwardIt last) {
    while (m_dst_cur != last && m_success) {
      m_success = bool_cast(vf(MyBase::default_construct()));
    }
  }

  void default_construct_n(size_t count) {
    for (size_t step = 0; step < count && m_success; ++step) {
      m_success = bool_cast(vf(MyBase::default_construct()));
    }
  }

  bool release() {
    if (m_success) {
      MyBase::release();
      return true;
    }
    return false;  // range will be destroyed
  }

 private:
  Verifier& m_vf;
  bool m_success;
};
}  // namespace mm::details

#ifndef KTL_NO_EXCEPTIONS
template <class ForwardIt, class Ty>
ForwardIt
#else
template <class Verifier, class ForwardIt, class Ty>
bool
#endif
uninitialized_fill(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    ForwardIt first,
    ForwardIt last,
    const Ty& val) {
  if constexpr (mm::details::memset_is_safe_v<Ty> && is_pointer_v<ForwardIt>) {
    memset(first, static_cast<int>(val),
           distance(first,
                    last))  // sizeof(Ty) must be equal to sizeof(unsigned char)
  } else {
    ForwardIt current{first};
#ifndef KTL_NO_EXCEPTIONS
    mm::details::uninitialized_construct_helper uconh(first);
#else
    mm::details::uninitialized_verified_construct_helper uconh(vf, first);
#endif
    vf.fill_range(last, val);
    return vf.release();
  }
}

#ifndef KTL_NO_EXCEPTIONS
template <class ForwardIt, class Ty>
ForwardIt
#else
template <class Verifier, class ForwardIt, class Ty>
bool
#endif
uninitialized_fill_n(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    ForwardIt first,
    size_t count,
    const Ty& val) {
  if constexpr (mm::details::memset_is_safe_v<Ty> && is_pointer_v<ForwardIt>) {
    memset(first, static_cast<int>(val),
           count)  // sizeof(Ty) must be equal to sizeof(unsigned char)
  } else {
    ForwardIt current{first};
#ifndef KTL_NO_EXCEPTIONS
    mm::details::uninitialized_construct_helper uconh(first);
#else
    mm::details::uninitialized_verified_construct_helper uconh(vf, first);
#endif
    vf.fill_n(count, val);
    return vf.release();
  }
}

#ifndef KTL_NO_EXCEPTIONS
template <class ForwardIt>
ForwardIt
#else
template <class Verifier, class ForwardIt>
bool
#endif
uninitialized_value_construct(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    ForwardIt first,
    ForwardIt last) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;
  if constexpr (is_trivially_default_constructible_v<value_type> &&
                is_pointer_v<ForwardIt>) {
    size_t bytes_count{distance(first, last) * sizeof(value_type)};
    memset(first, 0, bytes_count);
  } else {
    ForwardIt current{first};
#ifndef KTL_NO_EXCEPTIONS
    mm::details::uninitialized_construct_helper uconh(first);
#else
    mm::details::uninitialized_verified_construct_helper uconh(vf, first);
#endif
    vf.value_construct_range(last);
    return vf.release();
  }
}

#ifndef KTL_NO_EXCEPTIONS
template <class ForwardIt>
ForwardIt
#else
template <class Verifier, class ForwardIt>
bool
#endif
uninitialized_value_construct_n(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    ForwardIt first,
    size_t count) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;
  if constexpr (is_trivially_default_constructible_v<value_type> &&
                is_pointer_v<ForwardIt>) {
    memset(first, 0, count * sizeof(value_type));
  } else {
    ForwardIt current{first};
#ifndef KTL_NO_EXCEPTIONS
    mm::details::uninitialized_construct_helper uconh(first);
#else
    mm::details::uninitialized_verified_construct_helper uconh(vf, first);
#endif
    vf.value_construct_n(count);
    return vf.release();
  }
}

#ifndef KTL_NO_EXCEPTIONS
template <class ForwardIt>
ForwardIt
#else
template <class Verifier, class ForwardIt>
bool
#endif
uninitialized_default_construct(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    ForwardIt first,
    ForwardIt last) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;
  if constexpr (is_trivially_default_constructible_v<value_type> &&
                is_pointer_v<ForwardIt>) {
    size_t bytes_count{distance(first, last) * sizeof(value_type)};
    memset(first, 0, bytes_count);
  } else {
    ForwardIt current{first};
#ifndef KTL_NO_EXCEPTIONS
    mm::details::uninitialized_construct_helper uconh(first);
#else
    mm::details::uninitialized_verified_construct_helper uconh(vf, first);
#endif
    vf.default_construct_range(last);
    return vf.release();
  }
}

#ifndef KTL_NO_EXCEPTIONS
template <class ForwardIt>
ForwardIt
#else
template <class Verifier, class ForwardIt>
bool
#endif
uninitialized_default_construct_n(
#ifdef KTL_NO_EXCEPTIONS
    Verifier&& vf,
#endif
    ForwardIt first,
    size_t count) {
  using value_type = typename iterator_traits<ForwardIt>::value_type;
  if constexpr (is_trivially_default_constructible_v<value_type> &&
                is_pointer_v<ForwardIt>) {
    memset(first, 0, count * sizeof(value_type));
  } else {
    ForwardIt current{first};
#ifndef KTL_NO_EXCEPTIONS
    mm::details::uninitialized_construct_helper uconh(first);
#else
    mm::details::uninitialized_verified_construct_helper uconh(vf, first);
#endif
    vf.default_construct_n(count);
    return vf.release();
  }
}

}  // namespace winapi::kernel
#endif
