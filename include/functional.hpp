#pragma once
#include <functional_impl.hpp>

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <functional>
namespace ktl {
using minus;
using plus;
using reference_wrapper;
}  // namespace ktl
#else
#include <type_traits.hpp>
#include <utility.hpp>

namespace ktl {
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

template <>
struct minus<void> {
  template <class Ty1, class Ty2>
  constexpr auto operator()(Ty1&& lhs,
                            Ty2&& rhs) noexcept(noexcept(forward<Ty1>(lhs) -
                                                         forward<Ty2>(rhs)))
      -> decltype(forward<Ty1>(lhs) - forward<Ty2>(rhs)) {
    return forward<Ty1>(lhs) - forward<Ty2>(rhs);
  }
};

template <class Fn,
          class... Types,
          enable_if_t<is_invocable_v<Fn, Types...>, int> = 0>
constexpr invoke_result_t<Fn, Types...> invoke(
    Fn&& fn,
    Types&&... args) noexcept(is_nothrow_invocable_v<Fn, Types...>) {
  return fn::details::invoker<Fn>::invoke(forward<Fn>(fn),
                                          forward<Types>(args)...);
}

template <class Ret,
          class Fn,
          class... Types,
          enable_if_t<is_invocable_r_v<Ret, Fn, Types...>, int> = 0>
constexpr invoke_result_t<Fn, Types...> invoke(
    Fn&& fn,
    Types&&... args) noexcept(is_nothrow_invocable_r_v<F, Types...>) {
  if constexpr (is_void_v<Ret>) {
    invoke(forward<Fn>(fn), forward<Types>(args)...);
  } else {
    return invoke(forward<Fn>(fn), forward<Types>(args)...);
  }
}
}  // namespace ktl
#endif
