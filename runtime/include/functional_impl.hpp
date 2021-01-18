#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <functional>
namespace ktl {
using std::greater;
using std::less;
}  // namespace ktl
#else
namespace ktl {
template <class Ty = void>
struct less {
  constexpr bool operator()(const Ty& lhs, const Ty& rhs) const
      noexcept(noexcept(lhs < rhs)) {
    return lhs < rhs;
  }
};

template <>
struct less<void> {
  template <class Ty1, class Ty2>
  constexpr bool operator()(const Ty1& lhs, const Ty2& rhs) const
      noexcept(noexcept(lhs < rhs)) {
    return lhs < rhs;
  }

  using is_transparent = int;
};

template <class Ty = void>
struct greater {
  constexpr bool operator()(const Ty& lhs, const Ty& rhs) const
      noexcept(noexcept(lhs > rhs)) {
    return lhs > rhs;
  }
};

template <>
struct greater<void> {
  template <class Ty1, class Ty2>
  constexpr bool operator()(const Ty1& lhs, const Ty2& rhs) const
      noexcept(noexcept(lhs > rhs)) {
    return lhs > rhs;
  }

  using is_transparent = int;
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
struct prefix_decrement {
  constexpr auto operator()(Ty& val) noexcept(noexcept(--val))
      -> decltype(val) {
    return --val;
  }
};

template <>
struct prefix_decrement<void> {
  template <class Ty>
  constexpr auto operator()(Ty& val) noexcept(noexcept(--val))
      -> decltype(val) {
    return --val;
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
struct postfix_decrement {
  constexpr auto operator()(Ty& val) noexcept(noexcept(val--))
      -> decltype(val--) {
    return val--;
  }
};

template <>
struct postfix_decrement<void> {
  template <class Ty>
  constexpr auto operator()(Ty& val) noexcept(noexcept(val--))
      -> decltype(val--) {
    return val--;
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

template <class Ty = void>
struct minus {
  constexpr auto operator()(const Ty& lhs,
                            const Ty& rhs) noexcept(noexcept(lhs - rhs))
      -> decltype(lhs - rhs) {
    return lhs - rhs;
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
struct has_prefix_decrement : false_type {};

template <class Ty>
struct has_prefix_decrement<
    Ty,
    void_t<decltype(declval<prefix_decrement<> >()(declval<Ty>()))> >
    : true_type {};  //-- применим только к l-value

template <class Ty>
inline constexpr bool has_prefix_decrement_v = has_prefix_decrement<Ty>::value;

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

template <class Ty, class = void>
struct has_postfix_decrement : false_type {};

template <class Ty>
struct has_postfix_decrement<
    Ty,
    void_t<decltype(declval<postfix_decrement<> >()(declval<Ty>()))> >
    : true_type {  //-- применим только к l-value
};

template <class Ty>
inline constexpr bool has_postfix_decrement_v =
    has_postfix_decrement<Ty>::value;

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

template <class Ty1, class Ty2, class = void>
struct has_operator_minus : false_type {};

template <class Ty1, class Ty2>
struct has_operator_minus<
    Ty1,
    Ty2,
    void_t<decltype(declval<minus<> >()(declval<Ty1>(), declval<Ty2>()))> >
    : true_type {};

template <class Ty1, class Ty2>
inline constexpr bool has_operator_minus_v =
    has_operator_minus<Ty1, Ty2>::value;
}  // namespace ktl
#endif