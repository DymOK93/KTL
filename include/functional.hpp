#pragma once
#include <functional_impl.hpp>

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <functional>
namespace ktl {
using std::minus;
using std::plus;
using std::reference_wrapper;
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

namespace fn::details {
template <class Ty>
constexpr Ty& unrefwrap(Ty& ref) noexcept {
  return ref;
}
template <class Ty>
void unrefwrap(Ty&&) = delete;
}  // namespace fn::details

template <class Ty>
class reference_wrapper {
 public:
  using type = Ty;

 public:
  template <class U,
            enable_if_t<!is_same_v<reference_wrapper, remove_cvref_t<U> >,
                        decltype(fn::details::unrefwrap<Ty>(declval<U>()),
                                 int())> = 0>
  constexpr reference_wrapper(U&& value) noexcept(
      noexcept(fn::details::unrefwrap<Ty>(value)))
      : m_ptr(addressof(fn::details::unrefwrap<Ty>(forward<U>(value)))) {}

  reference_wrapper(const reference_wrapper&) noexcept = default;
  reference_wrapper& operator=(const reference_wrapper& other) noexcept =
      default;

  constexpr operator Ty&() const noexcept { return *m_ptr; }
  constexpr Ty& get() const noexcept { return *m_ptr; }

 private:
  Ty* m_ptr;
};

template <class Ty>
reference_wrapper(Ty&) -> reference_wrapper<Ty>;

template <class Ty>
constexpr reference_wrapper<Ty> ref(Ty& ref) noexcept {
  return reference_wrapper{ref};
}

template <class Ty>
constexpr reference_wrapper<Ty> ref(
    reference_wrapper<Ty> ref_wrapper) noexcept {
  return reference_wrapper{ref_wrapper.get()};
}

template <class Ty>
void ref(const Ty&&) = delete;

template <class Ty>
constexpr reference_wrapper<const Ty> cref(const Ty& ref) noexcept {
  return reference_wrapper<const Ty>{ref};
}

template <class Ty>
constexpr reference_wrapper<const Ty> cref(
    reference_wrapper<Ty> ref_wrapper) noexcept {
  return reference_wrapper<const Ty>{ref_wrapper.get()};
}

template <class Ty>
void cref(const Ty&&) = delete;
}  // namespace ktl
#endif
