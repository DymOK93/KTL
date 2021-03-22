#pragma once
#include <type_traits.hpp>
#include <utility.hpp>

namespace ktl {
namespace tt::details {
template <size_t Idx, class Ty>
struct tuple_element {
  Ty value{};
};

template <size_t Idx, template <size_t, class> class TupleElement, class Ty>
constexpr auto extract_type(TupleElement<Idx, Ty>& element) noexcept
    -> decltype(element.value);

template <size_t Idx, class Ty>
constexpr Ty& get_value(tuple_element<Idx, Ty>& element) {
  return element.value;
}

template <size_t Idx, class Ty>
constexpr const Ty& get_value(const tuple_element<Idx, Ty>& element) {
  return element.value;
}

template <size_t Idx, class Ty>
constexpr volatile Ty& get_value(volatile tuple_element<Idx, Ty>& element) {
  return element.value;
}

template <size_t Idx, class Ty>
constexpr const volatile Ty& get_value(
    const volatile tuple_element<Idx, Ty>& element) {
  return element.value;
}

template <size_t Idx, class Ty>
constexpr Ty&& get_value(tuple_element<Idx, Ty>&& element) {
  return forward<Ty>(element.value);
}

template <class IndexType, class... Types>
class tuple_base {};

template <size_t... Indices, class... Types>
class tuple_base<index_sequence<Indices...>, Types...>
    : public tuple_element<Indices, Types>... {
 public:
  template <size_t Idx, class U>
  using MyElementBase = tuple_element<Idx, U>;

 public:
  explicit constexpr tuple_base() noexcept(
      conjunction_v<is_nothrow_default_constructible<Types>...>) {}

  constexpr tuple_base(const tuple_base&) = default;
  constexpr tuple_base(tuple_base&&) = default;
  constexpr tuple_base& operator=(const tuple_base&) = default;
  constexpr tuple_base& operator=(tuple_base&&) = default;

  template <
      enable_if_t<conjunction_v<is_copy_constructible<Types>...>, int> = 0>
  constexpr tuple_base(const Types&... args) noexcept(
      conjunction_v<is_nothrow_copy_constructible<Types>...>)
      : MyElementBase<Indices, Types>{args}... {}

  template <
      class... OtherTypes,
      enable_if_t<sizeof...(OtherTypes) == sizeof...(Types) &&
                      conjunction_v<is_constructible<Types, OtherTypes>...>,
                  int> = 0>
  explicit constexpr tuple_base(OtherTypes&&... args) noexcept(
      conjunction_v<is_nothrow_constructible<Types, OtherTypes>...>)
      : MyElementBase<Indices, Types>{forward<OtherTypes>(args)}... {}

  template <class... OtherTypes,
            enable_if_t<sizeof...(OtherTypes) == sizeof...(Types) &&
                            conjunction<is_constructible<Types, OtherTypes>...>,
                        int> = 0>
  constexpr tuple_base(const tuple_base<OtherTypes...>& other)
      : MyElementBase<Indices, Types>{get_value<Indices>(other)}... {}

  template <class... OtherTypes,
            enable_if_t<sizeof...(OtherTypes) == sizeof...(Types) &&
                            conjunction<is_constructible<Types, OtherTypes>...>,
                        int> = 0>
  constexpr tuple_base(tuple_base<OtherTypes...>&& other)
      : MyElementBase<Indices, Types>{move(get_value<Indices>(other))}... {}

  template <class U1, class U2>
  constexpr tuple_base(const pair<U1, U2>& pair) noexcept(noexcept(
      is_nothrow_constructible_v<decltype(extract_type<0>(*this)),
                                 add_const_t<add_lvalue_reference_t<U1>>>&&
          is_nothrow_constructible_v<decltype(extract_type<1>(*this)),
                                     add_lvalue_reference_t<U2>>))
      : MyElementBase<0, decltype(extract_type<0>(*this))>{pair.first},
        MyElementBase<1, decltype(extract_type<1>(*this))>{pair.second} {}

  template <class U1, class U2>
  constexpr tuple_base(pair<U1, U2>&& pair) noexcept(
      noexcept(is_nothrow_constructible_v<decltype(extract_type<0>(*this)),
                                          add_rvalue_reference_t<U1>>&&
                   is_nothrow_constructible_v<decltype(extract_type<1>(*this)),
                                              add_rvalue_reference_t<U2>>))
      : MyElementBase<0, decltype(extract_type<0>(*this))>{move(pair.first)},
        MyElementBase<1, decltype(extract_type<1>(*this))>{move(pair.second)} {}

  template <class... OtherTypes>
  constexpr tuple_base& operator=(const tuple_base<OtherTypes...>& other) {
    if (addressof(this) != other) {
      ((get_value<Indices>(*this) = get_value<Indices>(other)), ...);
    }
    return *this;
  }

  template <class... OtherTypes>
  constexpr tuple_base& operator=(tuple_base<OtherTypes...>&& other) {
    if (addressof(this) != other) {
      ((get_value<Indices>(*this) = move(get_value<Indices>(other))), ...);
    }
    return *this;
  }

  template <class U1, class U2>
  constexpr tuple_base& operator=(const pair<U1, U2>& pair) {
    return assign(make_index_sequence<2>{}, pair.first, pair.second);
  }

  template <class U1, class U2>
  constexpr tuple_base& operator=(pair<U1, U2>&& pair) {
    return assign(make_index_sequence<2>{}, move(pair.first),
                  move(pair.second));
  }

  constexpr void swap(tuple_base& other) noexcept(
      conjunction_v<is_nothrow_swappable<Types>...>) {
    ((::swap(get_value<Indices>(*this), get_value<Indices>(other))), ...);
  }

 private:
  template <size_t... Indices, class... Types>
  tuple_base& assign(index_sequence<Indices...>, Types&&... args) {
    return assign_impl<Indices...>(forward<Types>(args)...);
  }

  template <size_t... Indices, class... Types>
  tuple_base& assign_impl(Types&&... args) {
    ((get_value<Indices>(*this) = forward<Types>(args)), ...);
    return *this;
  }
};
}  // namespace tt::details

template <class... Types>
class tuple
    : public tt::details::tuple_base<index_sequence_for<Types...>, Types...> {
 public:
  using MyBase =
      tt::details::tuple_base<index_sequence_for<Types...>, Types...>;

 public:
  using MyBase::MyBase;
  using MyBase::operator=;
  using MyBase::swap;
};

template <class... Types>
struct tuple_size;

template <class... Types>
struct tuple_size<tuple<Types...>>
    : public integral_constant<size_t, sizeof...(Types)> {};

template <class Ty>
inline constexpr size_t tuple_size_v = tuple_size<Ty>::value;

template <size_t Idx, class... Types>
constexpr decltype(auto) get(tuple<Types...>& target) noexcept {
  static_assert(Idx < tuple_size_v<tuple<Types...>>,
                "tuple index is out of bounds");
  return tt::details::get_value<Idx>(target);
}

template <size_t Idx, class... Types>
constexpr decltype(auto) get(const tuple<Types...>& target) noexcept {
  static_assert(Idx < tuple_size_v<tuple<Types...>>,
                "tuple index is out of bounds");
  return tt::details::get_value<Idx>(target);
}

template <size_t Idx, class... Types>
constexpr decltype(auto) get(const volatile tuple<Types...>& target) noexcept {
  static_assert(Idx < tuple_size_v<tuple<Types...>>,
                "tuple index is out of bounds");
  return tt::details::get_value<Idx>(target);
}

template <size_t Idx, class... Types>
constexpr decltype(auto) get(volatile tuple<Types...>& target) noexcept {
  static_assert(Idx < tuple_size_v<tuple<Types...>>,
                "tuple index is out of bounds");
  return tt::details::get_value<Idx>(target);
}

template <size_t Idx, class... Types>
constexpr decltype(auto) get(tuple<Types...>&& target) noexcept {
  static_assert(Idx < tuple_size_v<tuple<Types...>>,
                "tuple index is out of bounds");
  return tt::details::get_value<Idx>(move(target));
}

template <size_t Idx, class Ty>
using tuple_element = remove_reference<decltype(get<Idx>(declval<Ty>()))>;

template <class... Types>
tuple(Types...) -> tuple<Types...>;

// template <class Ty1, class Ty2>
// tuple(const pair<Ty1, Ty2>&) -> tuple<Ty1, Ty2>;
//
// template <class Ty1, class Ty2>
// tuple(pair<Ty1, Ty2>&&) -> tuple<Ty1, Ty2>;

template <class... Types>
void swap(tuple<Types...>& lhs, tuple<Types...>& rhs) noexcept(
    is_nothrow_swappable_v<tuple<Types...>>) {
  lhs.swap(rhs);
}

namespace tt::details {
template <class Ty>
struct unwrap_refwrapper {
  using type = Ty;
};

// template <class Ty>
// struct unwrap_refwrapper<reference_wrapper<Ty>> {
//  using type = Ty&;
//};

template <class Ty>
using unwrap_refwrapper_t = typename unwrap_refwrapper<Ty>::type;

template <class Ty>
using unwrap_decay_t = typename unwrap_refwrapper_t<decay_t<Ty>>;
}  // namespace tt::details

template <class... Types>
constexpr tuple<tt::details::unwrap_decay_t<Types>...> make_tuple(
    Types&&... args) {
  return tuple<tt::details::unwrap_decay_t<Types>...>(forward<Types>(args)...);
}

namespace tt::details {
struct ignore_tag {
  template <typename Ty>
  const ignore_tag& operator=(const Ty&) const noexcept {
    return *this;
  }
};
}  // namespace tt::details
inline constexpr tt::details::ignore_tag ignore;

template <class... Types>
[[nodiscard]] constexpr tuple<Types&...> tie(Types&... args) noexcept {
  return tuple<Types&...>(args...);
}

template <class... Types>
[[nodiscard]] constexpr tuple<Types&&...> forward_as_tuple(
    Types&&... args) noexcept {
  return tuple<Types&&...>(forward<Types>(args)...);
}

// namespace tt::details {
// template <class Ty, class... ForPairOrArray>
// struct view_as_tuple;
//
// template <class... Types>
// struct view_as_tuple<tuple<Types...>> {  // view a tuple as a tuple
//  using type = tuple<Types...>;
//};
//
// template <class Ty1, class Ty2>
// struct view_as_tuple<pair<Ty1, Ty2>> {  // view a pair as a tuple
//  using type = tuple<Ty1, Ty2>;
//};
//
// template <class... Types>
// using view_as_tuple_t = typename view_as_tuple<Types...>::type;
//
// template <size_t N, class Ty>
// struct repeat_for : integral_constant<size_t, N> {};
//
// template <class _Ret,
//          class _Kx_arg,
//          class _Ix_arg,
//          size_t _Ix_next,
//          class... Tuples>
// struct _Tuple_cat2 {  // determine tuple_cat's return type and _Kx/_Ix
// indices
//  static_assert(sizeof...(Tuples) == 0, "unsupported tuple_cat arguments");
//  using type = _Ret;
//  using _Kx_arg_seq = _Kx_arg;
//  using _Ix_arg_seq = _Ix_arg;
//};
//
// template <class... _Types1,
//          class _Kx_arg,
//          size_t... _Ix,
//          size_t _Ix_next,
//          class... _Types2,
//          class... _Rest>
// struct _Tuple_cat2<tuple<_Types1...>,
//                   _Kx_arg,
//                   index_sequence<_Ix...>,
//                   _Ix_next,
//                   tuple<_Types2...>,
//                   _Rest...>
//    : _Tuple_cat2<
//          tuple<_Types1..., _Types2...>,
//          typename _Cat_sequences<_Kx_arg,
//                                  index_sequence_for<_Types2...>>::type,
//          index_sequence<_Ix..., _Repeat_for<_Ix_next, _Types2>::value...>,
//          _Ix_next + 1,
//          _Rest...> {  // determine tuple_cat's return type and _Kx/_Ix
//          indices
//};
//
// template <class... Tuples>
// struct tuple_cat1
//    : _Tuple_cat2<tuple<>,
//                  index_sequence<>,
//                  index_sequence<>,
//                  0,
//                  typename view_as_tuple<decay_t<Tuples>>::type...> {};
//
// template <class ReturnType, size_t... TupleIdx, size_t... TyIdx, class Ty>
// constexpr ReturnType _Tuple_cat(index_sequence<TupleIdx...>,
//                                index_sequence<TyIdx...>,
//                                Ty&& arg) {
//  return ReturnType(get<TupleIdx>(get<TyIdx>(forward<Ty>(arg)))...);
//}
//}  // namespace tt::details
//
// template <class... Tuples>
//[[nodiscard]] constexpr typename tt::details::tuple_cat1<Tuples...>::type
// tuple_cat(Tuples&&... tuples) {
//  using _Cat1 = tt::details::tuple_cat1<_Tuples...>;
//  return tt::details::_Tuple_cat<typename _Cat1::type>(
//      typename _Cat1::_Kx_arg_seq(), typename _Cat1::_Ix_arg_seq(),
//      forward_as_tuple(forward<Tuples>(tuples)...));
//}

namespace tt::details {
template <size_t... Indices, class LhsTuple, class RhsTuple, class Predicate>
constexpr bool apply_to_tuple_pair_impl(const LhsTuple& lhs,
                                        const RhsTuple& rhs,
                                        Predicate pred) {
  return ((pred(get<Indices>(lhs), get<Indices>(rhs))), ...);
}

template <size_t... Indices, class LhsTuple, class RhsTuple, class Predicate>
constexpr bool apply_to_tuple_pair_proxy(const LhsTuple& lhs,
                                         const RhsTuple& rhs,
                                         Predicate pred,
                                         index_sequence<Indices...>) {
  return apply_to_tuple_pair_impl<Indices...>(lhs, rhs, pred);
}

template <class... LhsTypes,
          class... RhsTypes,
          class Predicate,
          enable_if_t<sizeof...(LhsTypes) == sizeof...(RhsTypes), int> = 0>
constexpr bool apply_to_tuple_pair(const tuple<LhsTypes...>& lhs,
                                   const tuple<RhsTypes...>& rhs,
                                   Predicate pred) {
  return apply_to_tuple_pair_proxy(lhs, rhs, pred,
                                   make_index_sequence<sizeof...(LhsTypes)>{});
}
}  // namespace tt::details

template <class... LhsTypes, class... RhsTypes>
constexpr bool operator==(const tuple<LhsTypes...>& lhs,
                          const tuple<RhsTypes...>& rhs) {
  return tt::details::apply_to_tuple_pair(
      lhs, rhs, [](const auto& lhs_val, const auto& rhs_val) {
        return lhs_val == rhs_val;
      });
}

template <class... LhsTypes, class... RhsTypes>
constexpr bool operator!=(const tuple<LhsTypes...>& lhs,
                          const tuple<RhsTypes...>& rhs) {
  return tt::details::apply_to_tuple_pair(
      lhs, rhs, [](const auto& lhs_val, const auto& rhs_val) {
        return lhs_val != rhs_val;
      });
}

template <class... LhsTypes, class... RhsTypes>
constexpr bool operator<(const tuple<LhsTypes...>& lhs,
                         const tuple<RhsTypes...>& rhs) {
  return tt::details::apply_to_tuple_pair(
      lhs, rhs, [](const auto& lhs_val, const auto& rhs_val) {
        return lhs_val < rhs_val;
      });
}

template <class... LhsTypes, class... RhsTypes>
constexpr bool operator<=(const tuple<LhsTypes...>& lhs,
                          const tuple<RhsTypes...>& rhs) {
  return tt::details::apply_to_tuple_pair(
      lhs, rhs, [](const auto& lhs_val, const auto& rhs_val) {
        return lhs_val <= rhs_val;
      });
}

template <class... LhsTypes, class... RhsTypes>
constexpr bool operator>(const tuple<LhsTypes...>& lhs,
                         const tuple<RhsTypes...>& rhs) {
  return tt::details::apply_to_tuple_pair(
      lhs, rhs, [](const auto& lhs_val, const auto& rhs_val) {
        return lhs_val > rhs_val;
      });
}

template <class... LhsTypes, class... RhsTypes>
constexpr bool operator>=(const tuple<LhsTypes...>& lhs,
                          const tuple<RhsTypes...>& rhs) {
  return tt::details::apply_to_tuple_pair(
      lhs, rhs, [](const auto& lhs_val, const auto& rhs_val) {
        return lhs_val >= rhs_val;
      });
}

}  // namespace ktl
