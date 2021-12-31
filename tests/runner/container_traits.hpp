#pragma once
#include <type_traits.hpp>
#include <utility.hpp>

namespace tests {
template <class Ty, class = void>
struct is_iterable : ktl::false_type {};

template <class Ty>
struct is_iterable<Ty,
                   ktl::void_t<decltype(ktl::declval<Ty>().begin()),
                               decltype(ktl::declval<Ty>().end())>>
    : ktl::true_type {};

template <class Ty>
inline constexpr bool is_iterable_v = is_iterable<Ty>::value;

template <class Ty>
struct range_based_for_supporting {
  static constexpr bool value{ktl::is_array_v<Ty> || is_iterable_v<Ty>};
};

template <class Ty>
inline constexpr bool range_based_for_supporting_v =
    range_based_for_supporting<Ty>::value;

template <class Ty, class = void>
struct has_traits : ktl::false_type {};

template <class Ty>
struct has_traits<Ty, ktl::void_t<typename Ty::traits_type>> : ktl::true_type {
};

template <class Ty>
inline constexpr bool has_traits_v = has_traits<Ty>::value;

template <class Ty, class = void>
struct has_value_type : ktl::false_type {};

template <class Ty>
struct has_value_type<Ty, ktl::void_t<typename Ty::value_type>>
    : ktl::true_type {};

template <class Ty>
inline constexpr bool has_value_type_v = has_value_type<Ty>::value;

template <class Ty, class = void>
struct has_key_type : ktl::false_type {};

template <class Ty>
struct has_key_type<Ty, ktl::void_t<typename Ty::key_type>> : ktl::true_type {};

template <class Ty>
inline constexpr bool has_key_type_v = has_key_type<Ty>::value;

template <class Ty, class = void>
struct has_mapped_type : ktl::false_type {};

template <class Ty>
struct has_mapped_type<Ty, ktl::void_t<typename Ty::mapped_type>>
    : ktl::true_type {};

template <class Ty>
inline constexpr bool has_mapped_type_v = has_mapped_type<Ty>::value;

template <class Ty>
struct is_container {
  static constexpr bool value{
      ktl::conjunction_v<has_value_type<Ty>, ktl::negation<has_traits<Ty>>>};
};

template <class Ty>
inline constexpr bool is_container_v = is_container<Ty>::value;

template <class Ty>
struct is_set {
  static constexpr bool value{
      ktl::conjunction_v<is_container<Ty>,
                         has_key_type<Ty>,
                         ktl::negation<has_mapped_type<Ty>>>};
};

template <class Ty>
inline constexpr bool is_set_v = is_set<Ty>::value;

template <class Ty>
struct is_map {
  static constexpr bool value{ktl::conjunction_v<is_container<Ty>,
                                                 has_key_type<Ty>,
                                                 has_mapped_type<Ty>>};
};

template <class Ty>
inline constexpr bool is_map_v = is_map<Ty>::value;

template <class Ty, class = void>
struct has_reserve : ktl::false_type {};

template <class Ty>
struct has_reserve<
    Ty,
    ktl::void_t<decltype(ktl::declval<Ty>().reserve(ktl::declval<size_t>()))>>
    : ktl::true_type {};

template <class Ty>
inline constexpr bool has_reserve_v = has_reserve<Ty>::value;

template <class Ty, class = void>
struct is_linear : ktl::false_type {};

template <class Ty>
struct is_linear<Ty,
                 ktl::void_t<decltype(ktl::declval<Ty>().push_back(
                     ktl::declval<typename Ty::value_type>()))>>
    : ktl::true_type {};

template <class Ty>
inline constexpr bool is_linear_v = is_linear<Ty>::value;

template <class Ty, class = void>
struct is_associative : ktl::false_type {};

template <class Ty>
struct is_associative<Ty,
                      ktl::void_t<decltype(ktl::declval<Ty>().insert(
                          ktl::declval<typename Ty::value_type>()))>>
    : ktl::true_type {};

template <class Ty>
inline constexpr bool is_associative_v = is_associative<Ty>::value;

template <class Ty, class = void>
struct data_type {
  using type = typename Ty::value_type;
};

template <class Ty>
struct data_type<Ty,
                 ktl::void_t<typename Ty::key_type, typename Ty::mapped_type>> {
  using type = ktl::pair<typename Ty::key_type, typename Ty::mapped_type>;
};

template <class Ty>
using data_type_t = typename data_type<Ty>::type;

template <class Container, class = void>
struct is_value_container {
  static constexpr bool value = false;
};

template <class Container>
struct is_value_container<
    Container,
    ktl::enable_if_t<is_linear_v<Container> || is_set_v<Container>>> {
  static constexpr bool value = true;
};

template <class Container>
struct is_value_container<Container, ktl::enable_if_t<is_map_v<Container>>> {
  static constexpr bool value =
      ktl::is_same_v<typename Container::mapped_type, void>;
};

template <class Container>
inline constexpr bool is_value_container_v =
    is_value_container<Container>::value;

template <class Container, class = void>
struct is_kv_container {
  static constexpr bool value = false;
};

template <class Container>
struct is_kv_container<Container, ktl::enable_if_t<is_map_v<Container>>> {
  static constexpr bool value =
      !ktl::is_same_v<typename Container::mapped_type, void>;
};

template <class Container>
inline constexpr bool is_kv_container_v = is_kv_container<Container>::value;
}  // namespace test
