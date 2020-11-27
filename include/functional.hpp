#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <functional>
namespace winapi::kernel {
using std::less;
using std::greater;
}  // namespace winapi::kernel
#else
#include <type_traits.hpp>
#include <utility.hpp>

namespace winapi::kernel {
template <class Ty = void>
struct less {
  constexpr bool operator()(const Ty& lhs, const Ty& rhs) const {
    return lhs < rhs;
  }
};

template <class Ty = void>
struct greater {
  constexpr bool operator()(const Ty& lhs, const Ty& rhs) const {
    return lhs > rhs;
  }
};

template <class Ty = void>
struct prefix_increment {
  constexpr auto operator()(Ty& val) noexcept(noexcept(++val))
      -> decltype(val) {
    return ++val;
  }
};

template <>
struct prefix_increment<void> {
  template <class Ty>
  constexpr auto operator()(Ty& val) noexcept(noexcept(++val))
      -> decltype(val) {
    return ++val;
  }
};

template <class Ty = void>
struct postfix_increment {
  constexpr auto operator()(Ty& val) noexcept(noexcept(val++))
      -> decltype(val++) {
    return val++;
  }
};

template <>
struct postfix_increment<void> {
  template <class Ty>
  constexpr auto operator()(Ty& val) noexcept(noexcept(val++))
      -> decltype(val++) {
    return val++;
  }
};

template <class Ty = void>
struct plus {
  constexpr auto operator()(const Ty& lhs,
                            const Ty& rhs) noexcept(noexcept(lhs + rhs))
      -> decltype(lhs + rhs) {
    return lhs + rhs;
  }
};

template <>
struct plus<void> {
  template <class Ty1, class Ty2>
  constexpr auto operator()(Ty1&& lhs,
                            Ty2&& rhs) noexcept(noexcept(forward<Ty1>(lhs) +
                                                         forward<Ty2>(rhs)))
      -> decltype(forward<Ty1>(lhs) + forward<Ty2>(rhs)) {
    return forward<Ty1>(lhs) + forward<Ty2>(rhs);
  }
};

template <class Ty, class = void>
struct has_prefix_increment : false_type {};

template <class Ty>
struct has_prefix_increment<
    Ty,
    void_t<decltype(declval<prefix_increment<> >()(declval<Ty>()))> >
    : true_type {};  //++ применим только к l-value

template <class Ty>
inline constexpr bool has_prefix_increment_v = has_prefix_increment<Ty>::value;

template <class Ty, class = void>
struct has_postfix_increment : false_type {};

template <class Ty>
struct has_postfix_increment<
    Ty,
    void_t<decltype(declval<postfix_increment<> >()(declval<Ty>()))> >
    : true_type {  //++ применим только к l-value
};

template <class Ty>
inline constexpr bool has_postfix_increment_v =
    has_postfix_increment<Ty>::value;

template <class Ty1, class Ty2, class = void>
struct has_operator_plus : false_type {};

template <class Ty1, class Ty2>
struct has_operator_plus<
    Ty1,
    Ty2,
    void_t<decltype(declval<plus<> >()(declval<Ty1>(), declval<Ty2>()))> >
    : true_type {};

template <class Ty1, class Ty2>
inline constexpr bool has_operator_plus_v = has_operator_plus<Ty1, Ty2>::value;

}  // namespace winapi::kernel
#endif