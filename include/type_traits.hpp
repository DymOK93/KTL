#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <type_traits>

namespace ktl {
using std::false_type;
using std::true_type;
using std::void_t;

using std::declval;

using std::enable_if;
using std::enable_if_t;
using std::conditional;
using std::conditional_t;
using std::common_type;
using std::common_type_t;

using std::remove_reference;
using std::remove_reference_t;
using std::add_lvalue_reference;
using std::add_lvalue_reference_t;
using std::add_rvalue_reference;
using std::add_rvalue_reference_t;
using std::add_pointer;
using std::add_pointer_t;
using std::add_const;
using std::add_const_t;

using std::is_convertible;
using std::is_convertible_v;
using std::is_constructible;
using std::is_constructible_v;
using std::is_default_constructible;
using std::is_default_constructible_v;
using std::is_copy_constructible;
using std::is_copy_constructible_v;
using std::is_move_constructible;
using std::is_move_constructible_v;
using std::is_nothrow_constructible;
using std::is_nothrow_constructible_v;
using std::is_nothrow_default_constructible;
using std::is_nothrow_default_constructible_v;
using std::is_nothrow_copy_constructible;
using std::is_nothrow_copy_constructible_v;
using std::is_nothrow_move_constructible;
using std::is_nothrow_move_constructible_v;

using std::is_assignable;
using std::is_assignable_v;
using std::is_copy_assignable;
using std::is_copy_assignable_v;
using std::is_move_assignable;
using std::is_move_assignable_v;
using std::is_nothrow_assignable;
using std::is_nothrow_assignable_v;
using std::is_nothrow_copy_assignable;
using std::is_nothrow_copy_assignable_v;
using std::is_nothrow_move_assignable;
using std::is_nothrow_move_assignable_v;

using std::is_swappable;
using std::is_swappable_v;
using std::is_nothrow_swappable;
using std::is_nothrow_swappable_v;

using std::is_array;
using std::is_array;
using std::is_pointer;
using std::is_pointer_v;
using std::is_lvalue_reference;
using std::is_lvalue_reference_v;
using std::is_rvalue_reference;
using std::is_lvalue_reference_v;
using std::is_reference;
using std::is_reference_v;
using std::is_same;
using std::is_same_v;

using std::is_trivially_copyable;
using std::is_trivially_destructible;

}  // namespace ktl
#else
namespace ktl {
template <class... Dummy>
struct always_false {
  static constexpr bool value = false;
};

template <class... Dummy>
inline constexpr bool always_false_v = always_false<Dummy...>::value;

struct false_type {
  static constexpr bool value = false;
};

struct true_type {
  static constexpr bool value = true;
};

template <class...>
using void_t = void;

template <class Ty>
Ty&& declval() noexcept;  //Только для SFINAE -
                          //определение не требуется

template <bool enable, class Ty>
struct enable_if {};

template <class Ty>
struct enable_if<true, Ty> {
  using type = Ty;
};

template <bool enable, class Ty>
using enable_if_t = typename enable_if<enable, Ty>::type;

template <bool flag, class Ty1, class Ty2>
struct conditional {
  using type = Ty1;
};

template <class Ty1, class Ty2>
struct conditional<false, Ty1, Ty2> {
  using type = Ty2;
};

template <bool flag, class Ty1, class Ty2>
using conditional_t = typename conditional<flag, Ty1, Ty2>::type;

template <class Ty>
struct remove_reference {
  using type = Ty;
};

template <class Ty>
struct remove_reference<Ty&> {
  using type = Ty;
};

template <class Ty>
struct remove_reference<Ty&&> {
  using type = Ty;
};

template <class Ty>
using remove_reference_t = typename remove_reference<Ty>::type;

template <class Ty>
struct remove_pointer {
  using type = Ty;
};

template <class Ty>
struct remove_pointer<Ty*> {
  using type = Ty;
};

template <class Ty>
using remove_pointer_t = typename remove_pointer<Ty>::type;

template <class Ty, class = void>
struct add_reference {
  using lvalue = Ty;  //Для void
  using rvalue = Ty;
};

template <class Ty>
struct add_reference<Ty,
                     void_t<Ty&> > {  //Для типов, на которые допустимы ссылки
  using lvalue = Ty&;
  using rvalue = Ty&&;
};

template <class Ty>
struct add_lvalue_reference {
  using type = typename add_reference<Ty>::lvalue;
};

template <class Ty>
using add_lvalue_reference_t = typename add_lvalue_reference<Ty>::type;

template <class Ty>
struct add_rvalue_reference {
  using type = typename add_reference<Ty>::rvalue;
};

template <class Ty>
using add_rvalue_reference_t = typename add_rvalue_reference<Ty>::type;

template <class Ty>
struct add_pointer {
  using type = Ty*;
};

template <class Ty>
using add_pointer_t = typename add_pointer<Ty>::type;

template <class Ty>
struct remove_const {
  using type = Ty;
};

template <class Ty>
struct remove_const<const Ty> {
  using type = Ty;
};

template <class Ty>
using remove_const_t = typename remove_const<Ty>::type;

template <class Ty>
struct add_const {
  using type = const Ty;
};

template <class Ty>
struct add_const<const Ty> {
  using type = const Ty;
};

template <class Ty>
using add_const_t = typename add_const<Ty>::type;

namespace tt::details {
template <class... Types>
struct common_type_impl {};

template <class Ty>
struct common_type_impl<Ty> {
  using type = Ty;
};

template <class Ty1, class Ty2>
struct common_type_impl<Ty1, Ty2> {
  using type = decltype(true ? declval<Ty1>() : declval<Ty2>());
};

template <class Ty1, class Ty2, class... Rest>
struct common_type_impl<Ty1, Ty2, Rest...> {
  using type =
      typename common_type_impl<typename common_type_impl<Ty1, Ty2>::type,
                                Rest...>::type;
};
}  // namespace tt::details

template <class... Types>
struct common_type {
  using type = typename tt::details::common_type_impl<Types...>::type;
};

template <class... Types>
using common_type_t = typename common_type<Types...>::type;

template <class From, class To, class = void>
struct is_convertible : false_type {};

template <class From, class To>
struct is_convertible<From,
                      To,
                      void_t<decltype(static_cast<To>(declval<From>()))> >
    : true_type {};

template <class From, class To>
inline constexpr bool is_convertible_v = is_convertible<From, To>::value;

template <class, class Ty, class... Types>
struct is_constructible : false_type {};

template <class Ty, class... Types>
struct is_constructible<void_t<decltype(Ty(declval<Types>()...))>, Ty, Types...>
    : true_type {};

template <class Ty, class... Types>
inline constexpr bool is_constructible_v =
    is_constructible<void_t<>, Ty, Types...>::value;

template <class Ty>
struct is_default_constructible {
  static constexpr bool value = is_constructible_v<Ty>;
};

template <class Ty>
inline constexpr bool is_default_constructible_v =
    is_default_constructible<Ty>::value;

template <class Ty>
struct is_trivially_default_constructible {
  static constexpr bool value = __is_trivially_constructible(Ty);
};

template <class Ty>
inline constexpr bool is_trivially_default_constructible_v =
    is_trivially_default_constructible<Ty>::value;

template <class Ty>
struct is_nothrow_default_constructible {
  static constexpr bool value = is_constructible_v<Ty>&& noexcept(Ty());
};

template <class Ty>
inline constexpr bool is_nothrow_default_constructible_v =
    is_default_constructible<Ty>::value;

template <class Ty>
struct is_copy_constructible {
  static constexpr bool value =
      is_constructible_v<Ty, add_const_t<add_lvalue_reference_t<Ty> > >;
};

template <class Ty>
inline constexpr bool is_copy_constructible_v =
    is_copy_constructible<Ty>::value;

template <class Ty>
struct is_move_constructible {
  static constexpr bool value =
      is_constructible_v<Ty, add_rvalue_reference_t<Ty> >;
};

template <class Ty>
inline constexpr bool is_move_constructible_v =
    is_move_constructible<Ty>::value;

template <class, class Ty, class... Types>
struct is_nothrow_constructible {
  static constexpr bool value =
      is_constructible_v<Ty, Types...>&& noexcept(Ty(declval<Types>()...));
};

template <class Ty, class... Types>
inline constexpr bool is_nothrow_constructible_v =
    is_nothrow_constructible<void_t<>, Ty, Types...>::value;

template <class Ty>
struct is_nothrow_copy_constructible {
  static constexpr bool value =
      is_constructible_v<Ty, add_const_t<add_lvalue_reference_t<Ty> > >&& noexcept(
          Ty(declval<add_const_t<add_lvalue_reference_t<Ty> > >()));
};

template <class Ty>
inline constexpr bool is_nothrow_copy_constructible_v =
    is_nothrow_copy_constructible<Ty>::value;

template <class Ty>
struct is_nothrow_move_constructible {
  static constexpr bool value =
      is_constructible_v<Ty, add_rvalue_reference_t<Ty> >&& noexcept(
          Ty(declval<add_rvalue_reference_t<Ty> >()));
  ;
};

template <class Ty>
inline constexpr bool is_nothrow_move_constructible_v =
    is_nothrow_move_constructible<Ty>::value;

template <class Ty, class Other, class = void>
struct is_assignable : false_type {};

template <class Ty, class Other>
struct is_assignable<Ty,
                     Other,
                     void_t<decltype(declval<Ty>() = declval<Other>())> >
    : true_type {};

template <class Ty, class Other>
inline constexpr bool is_assignable_v = is_assignable<Ty, Other>::value;

template <class Ty>
struct is_copy_assignable {
  static constexpr bool value =
      is_assignable_v<Ty, add_const_t<add_lvalue_reference_t<Ty> > >;
};

template <class Ty>
inline constexpr bool is_copy_assignable_v = is_copy_assignable<Ty>::value;

template <class Ty>
struct is_move_assignable {
  static constexpr bool value =
      is_assignable_v<Ty, add_rvalue_reference_t<Ty> >;
};

template <class Ty>
inline constexpr bool is_move_assignable_v = is_move_assignable<Ty>::value;

template <class Ty, class Other>
struct is_nothrow_assignable {
  static constexpr bool value =
      is_assignable_v<Ty, Other>&& noexcept(declval<Ty>() = declval<Other>());
};

template <class Ty, class Other>
inline constexpr bool is_nothrow_assignable_v =
    is_nothrow_assignable<Ty, Other>::value;

template <class Ty>
struct is_nothrow_copy_assignable {
  static constexpr bool value = is_copy_assignable_v<Ty>&& noexcept(
      declval<Ty>() = declval<add_const_t<add_lvalue_reference_t<Ty> > >());
};

template <class Ty>
inline constexpr bool is_nothrow_copy_assignable_v =
    is_nothrow_copy_assignable<Ty>::value;

template <class Ty>
struct is_nothrow_move_assignable {
  static constexpr bool value = is_move_assignable_v<Ty>&& noexcept(
      declval<Ty>() = declval<add_rvalue_reference_t<Ty> >());
};

template <class Ty>
inline constexpr bool is_nothrow_move_assignable_v =
    is_nothrow_move_assignable<Ty>::value;

template <class Ty>
struct is_swappable {
  static constexpr bool value =
      is_move_constructible_v<Ty> && is_move_assignable_v<Ty>;
};

template <class Ty>
inline constexpr bool is_swappable_v = is_swappable<Ty>::value;

template <class Ty>
struct is_nothrow_swappable {
  static constexpr bool value =
      is_nothrow_move_constructible_v<Ty> && is_nothrow_move_assignable_v<Ty>;
};

template <class Ty>
inline constexpr bool is_nothrow_swappable_v = is_nothrow_swappable<Ty>::value;

template <class Ty>
struct is_array : false_type {};

template <class Ty>
struct is_array<Ty[]> : true_type {};

template <class Ty>
inline constexpr bool is_array_v = is_array<Ty>::value;

template <class Ty>
struct is_pointer : false_type {};

template <class Ty>
struct is_pointer<Ty*> : true_type {};

template <class Ty>
inline constexpr bool is_pointer_v = is_pointer<Ty>::value;

template <class Ty>
struct is_lvalue_reference : false_type {};

template <class Ty>
struct is_lvalue_reference<Ty&> : true_type {};

template <class Ty>
struct is_lvalue_reference<Ty&&> : false_type {};

template <class Ty>
inline constexpr bool is_lvalue_reference_v = is_lvalue_reference<Ty>::value;

template <class Ty>
struct is_rvalue_reference : false_type {};

template <class Ty>
struct is_rvalue_reference<Ty&> : false_type {};

template <class Ty>
struct is_rvalue_reference<Ty&&> : true_type {};

template <class Ty>
inline constexpr bool is_rvalue_reference_v = is_rvalue_reference<Ty>::value;

template <class Ty>
struct is_reference {
  static constexpr bool value =
      is_lvalue_reference_v<Ty> || is_rvalue_reference_v<Ty>;
};
template <class Ty>
inline constexpr bool is_reference_v = is_reference<Ty>::value;

template <class Ty1, class Ty2>
struct is_same : false_type {};

template <class Ty>
struct is_same<Ty, Ty> : true_type {};

template <class Ty1, class Ty2>
inline constexpr bool is_same_v = is_same<Ty1, Ty2>::value;

template <class Ty>
struct is_const : false_type {};

template <class Ty>
struct is_const<const Ty> : true_type {};

template <class Ty>
inline constexpr bool is_const_v = is_const<Ty>::value;

template <class Ty>
struct is_trivially_copyable {
  static constexpr bool value = __is_trivially_copyable(Ty);
};

template <class Ty>
inline constexpr bool is_trivially_copyable_v =
    is_trivially_copyable<Ty>::value;

template <class Ty>
struct is_trivially_destructible {
  static constexpr bool value = __is_trivially_destructible(Ty);
};

template <class Ty>
inline constexpr bool is_trivially_destructible_v =
    is_trivially_copyable<Ty>::value;

}  // namespace ktl
#endif
