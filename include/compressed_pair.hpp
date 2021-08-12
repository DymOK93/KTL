#pragma once
#include <tuple.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

namespace ktl {
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
      is_nothrow_default_constructible_v<U1>&& 
          is_nothrow_default_constructible_v<U2>) {}

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
      : MyBase(piecewise_construct_t{}, {}, forward<Types>(args)...) {}

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
      : MyBase(piecewise_construct_t{},
               forward_as_tuple(forward<U1>(value)),
               forward_as_tuple(forward<Types>(args)...)) {}

  first_type& get_first() noexcept { return this->first; }
  const first_type& get_first() const noexcept { return this->first; }
  second_type& get_second() noexcept { return this->second; }
  const second_type& get_second() const noexcept { return this->second; }
};
}  // namespace ktl
