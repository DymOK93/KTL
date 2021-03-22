#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <utility>

namespace ktl {
using std::size;

using std::make_pair;
using std::pair;
}  // namespace ktl

#else
#include <intrinsic.hpp>
#include <type_traits.hpp>
#include <utility_impl.hpp>

namespace ktl {
template <class Ty>
constexpr const Ty* as_const(Ty* ptr) {
  return static_cast<add_pointer_t<add_const_t<Ty>>>(ptr);
}

template <class Ty>
constexpr const Ty& as_const(Ty& val) {
  return static_cast<add_lvalue_reference_t<add_const_t<Ty>>>(val);
}

template <class Container>
constexpr auto size(const Container& cont) -> decltype(cont.size()) {
  return cont.size();
}

template <class Ty, size_t N>
constexpr size_t size(const Ty (&arr)[N]) {
  return N;
}

// A custom pair implementation
template <typename Ty1, typename Ty2>
struct pair {
  using first_type = Ty1;
  using second_type = Ty2;

  template <typename U1 = Ty1,
            typename U2 = Ty2,
            typename =
                typename ktl::enable_if_t<ktl::is_default_constructible_v<U1> &&
                                          ktl::is_default_constructible_v<U2>>>
  constexpr pair() noexcept(is_nothrow_default_constructible_v<U1>&& noexcept(
      is_nothrow_default_constructible_v<U2>))
      : first(), second() {}

  constexpr pair(Ty1&& a,
                 Ty2&& b) noexcept(is_nothrow_move_constructible_v<Ty1>&&
                                       is_nothrow_move_constructible_v<Ty2>)
      : first(move(a)), second(move(b)) {}

  template <
      typename U1,
      typename U2,
      enable_if_t<is_constructible_v<Ty1, U1> && is_constructible_v<Ty2, U2>,
                  int> = 0>
  constexpr pair(U1&& a, U2&& b) noexcept(
      is_nothrow_constructible_v<Ty1, U1>&& is_nothrow_constructible_v<Ty2, U2>)
      : first(forward<U1>(a)), second(forward<U2>(b)) {}

  template <
      class U1,
      class U2,
      enable_if_t<is_constructible_v<Ty1, U1> && is_constructible_v<Ty2, U2>,
                  int> = 0>
  constexpr pair(const pair<U1, U2>& other) noexcept(
      is_nothrow_constructible_v<Ty1, const U1&>&&
          is_nothrow_constructible_v<Ty2, const U2&>)
      : first(other.first), second(other.second) {}

  template <
      class U1,
      class U2,
      enable_if_t<is_constructible_v<Ty1, U1> && is_constructible_v<Ty2, U2>,
                  int> = 0>
  constexpr pair(pair<U1, U2>&& other) noexcept(
      is_nothrow_constructible_v<Ty1, U1&&>&&
          is_nothrow_constructible_v<Ty2, U2&&>)
      : first(move(other.first)), second(move(other.second)) {}

  // template <typename... U1, typename... U2>
  // constexpr pair(
  //    ktl::piecewise_construct_t /*unused*/,
  //    ktl::tuple<U1...> a,
  //    ktl::tuple<U2...>
  //        b) noexcept(noexcept(pair(declval<ktl::tuple<U1...>&>(),
  //                                  declval<ktl::tuple<U2...>&>(),
  //                                  ROBIN_HOOD_STyD::index_sequence_for<U1...>(),
  //                                  ROBIN_HOOD_STyD::index_sequence_for<
  //                                      U2...>())))
  //    : pair(a,
  //           b,
  //           ROBIN_HOOD_STyD::index_sequence_for<U1...>(),
  //           ROBIN_HOOD_STyD::index_sequence_for<U2...>()) {}

  //// constructor called from the ktl::piecewise_construct_t ctor
  // template <typename... U1, size_t... I1, typename... U2, size_t... I2>
  // pair(ktl::tuple<U1...>& a, ktl::tuple<U2...>& b,
  // ROBIN_HOOD_STyD::index_sequence<I1...> /*unused*/,
  // ROBIN_HOOD_STyD::index_sequence<I2...> /*unused*/) noexcept(
  //    noexcept(Ty1(forward<U1>(ktl::get<I1>(
  //        declval<ktl::tuple<
  //            U1...>&>()))...)) && noexcept(Ty2(ktl::
  //                                                 forward<U2>(ktl::get<I2>(
  //                                                     declval<ktl::tuple<
  //                                                         U2...>&>()))...)))
  //    : first(forward<U1>(ktl::get<I1>(a))...),
  //      second(forward<U2>(ktl::get<I2>(b))...) {
  //  // make visual studio compiler happy about warning about unused a & b.
  //  // Visual studio's pair implementation disables warning 4100.
  //  (void)a;
  //  (void)b;
  //}

  void swap(pair<Ty1, Ty2>& o) noexcept(
      is_nothrow_swappable_v<Ty1>&& is_nothrow_swappable_v<Ty2>) {
    swap(first, o.first);
    swap(second, o.second);
  }

  Ty1 first;   // NOLINTy(misc-non-private-member-variables-in-classes)
  Ty2 second;  // NOLINTy(misc-non-private-member-variables-in-classes)
};

template <typename Ty1, typename Ty2>
void swap(pair<Ty1, Ty2>& a, pair<Ty1, Ty2>& b) noexcept(
    noexcept(declval<pair<Ty1, Ty2>&>().swap(declval<pair<Ty1, Ty2>&>()))) {
  a.swap(b);
}

template <typename Ty1, typename Ty2>
constexpr bool operator==(pair<Ty1, Ty2> const& x, pair<Ty1, Ty2> const& y) {
  return (x.first == y.first) && (x.second == y.second);
}
template <typename Ty1, typename Ty2>
constexpr bool operator!=(pair<Ty1, Ty2> const& x, pair<Ty1, Ty2> const& y) {
  return !(x == y);
}
template <typename Ty1, typename Ty2>
constexpr bool
operator<(pair<Ty1, Ty2> const& x, pair<Ty1, Ty2> const& y) noexcept(
    noexcept(declval<Ty1 const&>() <
             declval<Ty1 const&>()) && noexcept(declval<Ty2 const&>() <
                                                declval<Ty2 const&>())) {
  return x.first < y.first || (!(y.first < x.first) && x.second < y.second);
}
template <typename Ty1, typename Ty2>
constexpr bool operator>(pair<Ty1, Ty2> const& x, pair<Ty1, Ty2> const& y) {
  return y < x;
}
template <typename Ty1, typename Ty2>
constexpr bool operator<=(pair<Ty1, Ty2> const& x, pair<Ty1, Ty2> const& y) {
  return !(x > y);
}
template <typename Ty1, typename Ty2>
inline constexpr bool operator>=(pair<Ty1, Ty2> const& x,
                                 pair<Ty1, Ty2> const& y) {
  return !(x < y);
}

namespace tt::details {
template <class Ty>
struct uncvref {
  using type = remove_cv_t<remove_reference_t<Ty>>;
};
template <class Ty>
using uncvref_t = typename uncvref<Ty>::type;
}  // namespace tt::details

template <class Ty1, class Ty2>
[[nodiscard]] constexpr pair<tt::details::uncvref_t<Ty1>,
                             tt::details::uncvref_t<Ty2>>
make_pair(Ty1&& first, Ty2&& second) noexcept(
    is_nothrow_constructible_v<tt::details::uncvref_t<Ty1>, Ty1>&&
        is_nothrow_constructible_v<tt::details::uncvref_t<Ty2>,
                                   Ty2>) /* strengthened */ {
  // return pair composed from arguments
  using pair_type =
      pair<tt::details::uncvref_t<Ty1>, tt::details::uncvref_t<Ty2>>;
  return pair_type(forward<Ty1>(first), forward<Ty2>(second));
}
}  // namespace ktl
#endif

namespace ktl {
template <class Ty, class U>
constexpr auto* pointer_cast(U* ptr) noexcept {
  using pvoid_type = conditional_t<is_const_v<U>, const void*, void*>;
  using pointer_type =
      conditional_t<is_const_v<U>, add_pointer_t<add_const_t<Ty>>,
                    add_pointer_t<Ty>>;

  return static_cast<pointer_type>(static_cast<pvoid_type>(ptr));
}

template <class Ty>
constexpr bool bool_cast(Ty&& val) noexcept(
    noexcept(static_cast<bool>(forward<Ty>(val)))) {
  return static_cast<bool>(forward<Ty>(val));
}

template <typename T>
T rotr(T x, unsigned k) {
  return (x >> k) | (x << (8U * sizeof(T) - k));
}

// This cast gets rid of warnings like "cast from 'uint8_t*' {aka 'unsigned
// char*'} to 'uint64_t*' {aka 'long unsigned int*'} increases required
// alignment of target type". Use with care!
template <typename Ty>
constexpr Ty reinterpret_cast_no_cast_align_warning(void* ptr) noexcept {
  return reinterpret_cast<Ty>(ptr);
}

template <typename Ty>
constexpr Ty reinterpret_cast_no_cast_align_warning(const void* ptr) noexcept {
  return reinterpret_cast<Ty>(ptr);
}

template <typename Ty>
Ty unaligned_load(const void* ptr) noexcept {
  // Using memcpy so we don't get into unaligned load problems.
  // Compiler should optimize this very well anyways.
  Ty value;
  memcpy(addressof(value), ptr, sizeof(Ty));
  return value;
}

struct zero_then_variadic_args {};
struct one_then_variadic_args {};

template <class Ty1, class Ty2, bool = is_empty_v<Ty1> && !is_final_v<Ty1>>
class compressed_pair : private Ty1 {
 public:
  using MyBase = Ty1;

  using first_type = Ty1;
  using second_type = Ty2;

 public:
  template <typename U1 = Ty1,
            typename U2 = Ty2,
            typename =
                typename ktl::enable_if_t<ktl::is_default_constructible_v<U1> &&
                                          ktl::is_default_constructible_v<U2>>>
  constexpr compressed_pair() noexcept(
      is_nothrow_default_constructible_v<U1>&& noexcept(
          is_nothrow_default_constructible_v<U2>)) {}

  constexpr compressed_pair(Ty1&& a, Ty2&& b) noexcept(
      is_nothrow_move_constructible_v<Ty1>&&
          is_nothrow_move_constructible_v<Ty2>)
      : MyBase(move(a)), m_value(move(b)) {}

  template <class... Types,
            enable_if_t<is_constructible_v<Ty2, Types...> &&
                            is_default_constructible_v<Ty1>,
                        int> = 0>
  constexpr compressed_pair(zero_then_variadic_args, Types&&... args) noexcept(
      is_nothrow_default_constructible_v<Ty1>&&
          is_nothrow_constructible_v<Ty2, Types...>)
      : m_value(forward<Types>(args)...) {}

  template <class U1,
            class... Types,
            enable_if_t<is_constructible_v<Ty1, U1> &&
                            is_constructible_v<Ty2, Types...>,
                        int> = 0>
  constexpr compressed_pair(
      one_then_variadic_args,
      U1&& value,
      Types&&... args) noexcept(is_nothrow_default_constructible_v<Ty1>&&
                                    is_nothrow_constructible_v<Ty2, Types...>)
      : MyBase(forward<U1>(value)), m_value(forward<Types>(args)...) {}

  template <
      typename U1,
      typename U2,
      enable_if_t<is_constructible_v<Ty1, U1> && is_constructible_v<Ty2, U2>,
                  int> = 0>
  constexpr compressed_pair(U1&& a, U2&& b) noexcept(
      is_nothrow_constructible_v<Ty1, U1>&& is_nothrow_constructible_v<Ty2, U2>)
      : MyBase(forward<U1>(a)), m_value(forward<U2>(b)) {}

  template <
      class U1,
      class U2,
      enable_if_t<is_constructible_v<Ty1, U1> && is_constructible_v<Ty2, U2>,
                  int> = 0>
  constexpr compressed_pair(const compressed_pair<U1, U2>& other) noexcept(
      is_nothrow_constructible_v<Ty1, const U1&>&&
          is_nothrow_constructible_v<Ty2, const U2&>)
      : MyBase(other.first), m_value(other.second) {}

  template <
      class U1,
      class U2,
      enable_if_t<is_constructible_v<Ty1, U1> && is_constructible_v<Ty2, U2>,
                  int> = 0>
  constexpr compressed_pair(compressed_pair<U1, U2>&& other) noexcept(
      is_nothrow_constructible_v<Ty1, U1&&>&&
          is_nothrow_constructible_v<Ty2, U2&&>)
      : MyBase(move(other.first)), m_value(move(other.second)) {}

  first_type& get_first() noexcept { return static_cast<MyBase&>(*this); }

  const first_type& get_first() const noexcept {
    return static_cast<const MyBase&>(*this);
  }

  second_type& get_second() noexcept { return m_value; }
  const second_type& get_second() const noexcept { return m_value; }

 private:
  Ty2 m_value{};
};

template <class Ty1, class Ty2>
class compressed_pair<Ty1, Ty2, false> : private pair<Ty1, Ty2> {
 public:
  using MyBase = pair<Ty1, Ty2>;

  using first_type = typename MyBase::first_type;
  using second_type = typename MyBase::second_type;

 public:
  using MyBase::MyBase;

  template <class... Types,
            enable_if_t<is_constructible_v<Ty2, Types...> &&
                            is_default_constructible_v<Ty1>,
                        int> = 0>
  constexpr compressed_pair(zero_then_variadic_args, Types&&... args) noexcept(
      is_nothrow_default_constructible_v<Ty1>&&
          is_nothrow_constructible_v<Ty2, Types...>)
      : m_value(forward<Types>(args)...) {}

  template <class U1,
            class... Types,
            enable_if_t<is_constructible_v<Ty1, U1> &&
                            is_constructible_v<Ty2, Types...>,
                        int> = 0>
  constexpr compressed_pair(
      one_then_variadic_args,
      U1&& value,
      Types&&... args) noexcept(is_nothrow_default_constructible_v<Ty1>&&
                                    is_nothrow_constructible_v<Ty2, Types...>)
      : MyBase(forward<U1>(value)), m_value(forward<Types>(args)...) {}

  first_type& get_first() noexcept { return this->first; }
  const first_type& get_first() const noexcept { return this->first; }
  second_type& get_second() noexcept { return this->second; }
  const second_type& get_second() const noexcept { return this->second; }
};

struct piecewise_construct_t {
  explicit piecewise_construct_t() = default;
};
inline constexpr piecewise_construct_t piecewise_construct{};
}  // namespace ktl
