#pragma once
#include <algorithm.hpp>
#include <assert.hpp>
#include <basic_types.hpp>
#include <container_helpers.hpp>
#include <ktlexcept.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

namespace ktl {
template <class Ty, size_t Size>
struct array {
  using value_type = Ty;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using pointer = Ty*;
  using const_pointer = const Ty*;
  using reference = Ty&;
  using const_reference = const Ty&;

  using iterator = pointer;
  using const_iterator = const_pointer;

  // using reverse_iterator =  reverse_iterator<iterator>;
  // using const_reverse_iterator = reverse_iterator<const_iterator>;

  void fill(const Ty& value) { fill_n(m_elems, Size, value); }

  void swap(array& other) noexcept(is_nothrow_swappable_v<Ty>) {
    swap_ranges(m_elems, m_elems + Size, other.m_elems);
  }

  [[nodiscard]] constexpr iterator begin() noexcept { return m_elems; }

  [[nodiscard]] constexpr const_iterator begin() const noexcept {
    return m_elems;
  }

  [[nodiscard]] constexpr iterator end() noexcept { return m_elems + Size; }

  [[nodiscard]] constexpr const_iterator end() const noexcept {
    return m_elems + Size;
  }

  //[[nodiscard]] constexpr reverse_iterator rbegin() noexcept {
  //  return reverse_iterator(end());
  //}

  //[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept {
  //  return const_reverse_iterator(end());
  //}

  // [[nodiscard]] constexpr reverse_iterator rend() noexcept {
  //  return reverse_iterator(begin());
  //}

  //[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept {
  //  return const_reverse_iterator(begin());
  //}

  [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
    return begin();
  }

  [[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

  //[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
  //  return rbegin();
  //}

  //[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
  //  return rend();
  //}

  [[nodiscard]] constexpr size_type size() const noexcept { return Size; }
  [[nodiscard]] constexpr size_type max_size() const noexcept { return Size; }
  [[nodiscard]] constexpr bool empty() const noexcept { return false; }

  [[nodiscard]] constexpr reference at(size_type idx) {
    return cont::details::at_index_verified(m_elems, idx, Size);
  }

  [[nodiscard]] constexpr const_reference at(size_type idx) const {
    return cont::details::at_index_verified(m_elems, idx, Size);
  }

  [[nodiscard]] constexpr reference operator[](size_type idx) noexcept {
    assert_with_msg(idx < Size, "index is out of range");
    return m_elems[idx];
  }

  [[nodiscard]] constexpr const_reference operator[](
      size_type idx) const noexcept {
    assert_with_msg(idx < Size, "index is out of range");
    return m_elems[idx];
  }

  [[nodiscard]] constexpr reference front() noexcept { return m_elems[0]; }

  [[nodiscard]] constexpr const_reference front() const noexcept {
    return m_elems[0];
  }

  [[nodiscard]] constexpr reference back() noexcept {
    return m_elems[Size - 1];
  }

  [[nodiscard]] constexpr const_reference back() const noexcept {
    return m_elems[Size - 1];
  }

  [[nodiscard]] constexpr Ty* data() noexcept { return m_elems; }
  [[nodiscard]] constexpr const Ty* data() const noexcept { return m_elems; }

  Ty m_elems[Size];
};

namespace cont::details {
template <class First, class... Rest>
struct enforce_same {
  static_assert((is_same_v<First, Rest> && ...),
                "N4687 26.3.7.2 [array.cons]/2: Requires: (is_same_v<T, U> && "
                "...) is true. Otherwise the program is ill-formed");
  using type = First;
};

template <class First, class... Rest>
using enforce_same_t = typename enforce_same<First, Rest...>::type;
}  // namespace cont::details

template <class First, class... Rest>
array(First, Rest...) -> array<cont::details::enforce_same_t<First, Rest...>,
                               1 + sizeof...(Rest)>;

template <class Ty>
struct array<Ty, 0> {
  using value_type = Ty;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using pointer = Ty*;
  using const_pointer = const Ty*;
  using reference = Ty&;
  using const_reference = const Ty&;

  using iterator = pointer;
  using const_iterator = const_pointer;

  // using reverse_iterator = reverse_iterator<iterator>;
  // using const_reverse_iterator = reverse_iterator<const_iterator>;

  void fill(const Ty&) {}
  void swap(array&) noexcept {}

  [[nodiscard]] constexpr iterator begin() noexcept { return iterator{}; }

  [[nodiscard]] constexpr const_iterator begin() const noexcept {
    return const_iterator{};
  }

  [[nodiscard]] constexpr iterator end() noexcept { return iterator{}; }

  [[nodiscard]] constexpr const_iterator end() const noexcept {
    return const_iterator{};
  }

  //[[nodiscard]] constexpr reverse_iterator rbegin() noexcept {
  //  return reverse_iterator(end());
  //}

  //[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept {
  //  return const_reverse_iterator(end());
  //}

  //[[nodiscard]] constexpr reverse_iterator rend() noexcept {
  //  return reverse_iterator(begin());
  //}

  //[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept {
  //  return const_reverse_iterator(begin());
  //}

  [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
    return begin();
  }

  [[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

  //[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
  //  return rbegin();
  //}

  //[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
  //  return rend();
  //}

  [[nodiscard]] constexpr size_type size() const noexcept { return 0ull; }
  [[nodiscard]] constexpr size_type max_size() const noexcept { return 0ull; }
  [[nodiscard]] constexpr bool empty() const noexcept { return true; }

  [[noreturn]] reference at(size_type) { throw_has_no_elements(); }
  [[noreturn]] const_reference at(size_type) const { throw_has_no_elements(); }

  [[nodiscard]] reference operator[](size_type) noexcept {
    crt_assert_with_msg(
        false, "operator[] call is invalid: array<Ty, 0> has no elements");
    return m_elems[0];
  }

  [[nodiscard]] reference operator[](size_type) const noexcept {
    crt_assert_with_msg(
        false, "operator[]() call is invalid: array<Ty, 0> has no elements");
    return m_elems[0];
  }

  [[nodiscard]] reference front() noexcept /* strengthened */ {
    crt_assert_with_msg(
        false, "front() call is invalid: array<Ty, 0> has no elements");
    return m_elems[0];
  }

  [[nodiscard]] const_reference front() const noexcept /* strengthened */ {
    crt_assert_with_msg(
        false, "front() call is invalid: array<Ty, 0> has no elements");
    return m_elems[0];
  }

  [[nodiscard]] reference back() noexcept /* strengthened */ {
    crt_assert_with_msg(false,
                        "back() call is invalid: array<Ty, 0> has no elements");
    return m_elems[0];
  }

  [[nodiscard]] const_reference back() const noexcept /* strengthened */ {
    crt_assert_with_msg(false,
                        "back() call is invalid: array<Ty, 0> has no elements");
    return m_elems[0];
  }

  [[nodiscard]] constexpr Ty* data() noexcept { return nullptr; }
  [[nodiscard]] constexpr const Ty* data() const noexcept { return nullptr; }

  Ty m_elems[1];

 private:
  [[noreturn]] static void throw_has_no_elements() {
    throw_exception<out_of_range>("array<Ty, 0> has no elements");
  }
};

template <class Ty,
          size_t Size,
          enable_if_t<Size == 0 || is_swappable_v<Ty>, int> = 0>
void swap(array<Ty, Size>& lhs, array<Ty, Size>& rhs) noexcept(
    is_nothrow_swappable_v<array<Ty, Size>>) {
  return lhs.swap(rhs);
}

template <class Ty, size_t Size>
[[nodiscard]] bool operator==(const array<Ty, Size>& lhs,
                              const array<Ty, Size>& rhs) {
  return equal(begin(lhs), end(lhs), begin(rhs));
}

template <class Ty, size_t Size>
[[nodiscard]] bool operator!=(const array<Ty, Size>& lhs,
                              const array<Ty, Size>& rhs) {
  return !(lhs == rhs);
}

template <class Ty, size_t Size>
[[nodiscard]] bool operator<(const array<Ty, Size>& lhs,
                             const array<Ty, Size>& rhs) {
  return lexicographical_compare(begin(lhs), end(lhs), begin(rhs), end(rhs));
}

template <class Ty, size_t Size>
[[nodiscard]] bool operator>(const array<Ty, Size>& lhs,
                             const array<Ty, Size>& rhs) {
  return rhs < lhs;
}

template <class Ty, size_t Size>
[[nodiscard]] bool operator<=(const array<Ty, Size>& lhs,
                              const array<Ty, Size>& rhs) {
  return !(rhs < lhs);
}

template <class Ty, size_t Size>
[[nodiscard]] bool operator>=(const array<Ty, Size>& lhs,
                              const array<Ty, Size>& rhs) {
  return !(lhs < rhs);
}

#define KTL_EMPTY_TAG /* Nothing here */
#define KTL_MOVE(val) move(val)
#define KTL_NO_MOVE(val) val

#define KTL_ARRAY_GET_IMPL(Const, Volatile, Ref, Move)      \
  template <size_t Idx, class Ty, size_t Size>              \
  constexpr decltype(auto) get(                             \
      Const Volatile array<Ty, Size> Ref target) noexcept { \
    return Move(target.m_elems[Idx]);                       \
  }

KTL_ARRAY_GET_IMPL(KTL_EMPTY_TAG, KTL_EMPTY_TAG, &,  KTL_NO_MOVE)
KTL_ARRAY_GET_IMPL(const,         KTL_EMPTY_TAG, &,  KTL_NO_MOVE)
KTL_ARRAY_GET_IMPL(KTL_EMPTY_TAG, volatile,      &,  KTL_NO_MOVE)
KTL_ARRAY_GET_IMPL(const,         volatile,      &,  KTL_NO_MOVE)
KTL_ARRAY_GET_IMPL(KTL_EMPTY_TAG, KTL_EMPTY_TAG, &&, KTL_MOVE)
KTL_ARRAY_GET_IMPL(const,         KTL_EMPTY_TAG, &&, KTL_MOVE)
KTL_ARRAY_GET_IMPL(KTL_EMPTY_TAG, volatile,      &&, KTL_MOVE)
KTL_ARRAY_GET_IMPL(const,         volatile,      &&, KTL_MOVE)

#define KTL_TO_ARRAY_IMPL(Ref, Move, Trait, Msg)                             \
  namespace cont::details {                                                  \
  template <class Ty, size_t Size, size_t... Idx>                            \
  [[nodiscard]] constexpr array<remove_cv_t<Ty>, Size> to_array_impl(        \
      Ty(Ref arr)[Size],                                                     \
      index_sequence<Idx...>) {                                              \
    return {{Move(arr[Idx])...}};                                            \
  }                                                                          \
  }                                                                          \
                                                                             \
  template <class Ty, size_t Size, size_t... Idx>                            \
  [[nodiscard]] constexpr array<remove_cv_t<Ty>, Size> to_array(             \
      Ty(Ref arr)[Size]) {                                                   \
    static_assert(!is_array_v<Ty>,                                           \
                  "N4830 [array.creation]/1: to_array does not accept "      \
                  "multidimensional arrays");                                \
    static_assert(Trait, "N4830 [array.creation]/1: to_array requires " #Msg \
                         " constructible elements");                         \
    return cont::details::to_array_impl(Move(arr),                           \
                                        make_index_sequence<Size>{});        \
  }

KTL_TO_ARRAY_IMPL(&,  KTL_EMPTY_TAG, (is_constructible_v<Ty, Ty&>), copy)
KTL_TO_ARRAY_IMPL(&&, KTL_MOVE,       is_move_constructible_v<Ty>,  move)

#undef KTL_TO_ARRAY_IMPL
#undef KTL_ARRAY_GET_IMPL
#undef KTL_NO_MOVE
#undef KTL_MOVE
#undef KTL_EMPTY_TAG
}  // namespace ktl