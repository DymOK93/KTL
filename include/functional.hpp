#pragma once
#include <functional_impl.hpp>

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <functional>
namespace ktl {}  // namespace ktl
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
}  // namespace ktl
#endif
