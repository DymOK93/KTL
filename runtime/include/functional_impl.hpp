#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <functional>
namespace ktl {
using std::equal_to;
using std::greater;
using std::less;
}  // namespace ktl
#else
#include <type_traits_impl.hpp>

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
struct equal_to {
  constexpr bool operator()(const Ty& lhs, const Ty& rhs) const
      noexcept(noexcept(lhs == rhs)) {
    return lhs == rhs;
  }
};

template <>
struct equal_to<void> {
  template <class Ty1, class Ty2>
  constexpr bool operator()(const Ty1& lhs, const Ty2& rhs) const
      noexcept(noexcept(lhs == rhs)) {
    return lhs == rhs;
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

template <class>
constexpr bool is_reference_wrapper_v = false;

template <class Ty>
constexpr bool is_reference_wrapper_v<reference_wrapper<Ty> > = true;

namespace fn::details {
enum class CallTargetCategory : uint8_t {
  Object,
  ReferenceWrapper,
  Pointer,
  Undefined
};

template <class C, class Ty>
constexpr CallTargetCategory get_call_target_category() noexcept {
  if constexpr (is_base_of_v<C, decay_t<Ty> >) {
    return CallTargetCategory::Object;
  } else if constexpr (is_reference_wrapper_v<decay_t<Ty> >) {
    return CallTargetCategory::ReferenceWrapper;
  } else if constexpr (is_dereferenceable_v<Ty>) {
    if constexpr (get_call_target_category<C, decltype(*declval<Ty>())>() ==
                  CallTargetCategory::Object) {
      return CallTargetCategory::Pointer;
    } else {
      return CallTargetCategory::Undefined;
    }
  } else {
    return CallTargetCategory::Undefined;
  }
}

template <CallTargetCategory Category>
struct member_ptr_invoker;

template <>
struct member_ptr_invoker<CallTargetCategory::Object> {
  template <class C,
            class Pointed,
            class Ty,
            class... Types,
            enable_if_t<is_function_v<Pointed>, int> = 0>
  static decltype(auto) invoke(
      Pointed C::*fn,
      Ty&& obj,
      Types&&... args) noexcept(noexcept((forward<Ty>(obj).*
                                          fn)(forward<Types>(args)...))) {
    return (forward<Ty>(obj).*fn)(forward<Types>(args)...);
  }

  template <
      class C,
      class Pointed,
      class Ty,
      class... Types,
      enable_if_t<!is_function_v<Pointed> && sizeof...(Types) == 0, int> = 0>
  static decltype(auto) invoke(
      Pointed C::*fn,
      Ty&& obj,
      Types&&... args) noexcept(noexcept(forward<Ty>(obj).*fn)) {
    return forward<Ty>(obj).*fn;
  }
};

template <>
struct member_ptr_invoker<CallTargetCategory::ReferenceWrapper> {
  template <class C,
            class Pointed,
            class Ty,
            class... Types,
            enable_if_t<is_function_v<Pointed>, int> = 0>
  static decltype(auto) invoke(
      Pointed C::*fn,
      Ty&& obj,
      Types&&... args) noexcept(noexcept((obj.get().*
                                          fn)(forward<Types>(args)...))) {
    return (obj.get().*fn)(forward<Types>(args)...);
  }

  template <
      class C,
      class Pointed,
      class Ty,
      class... Types,
      enable_if_t<!is_function_v<Pointed> && sizeof...(Types) == 0, int> = 0>
  static decltype(auto) invoke(Pointed C::*fn,
                               Ty&& obj,
                               Types&&... args) noexcept(noexcept(obj.get().*
                                                                  fn)) {
    return obj.get().*fn;
  }
};

template <>
struct member_ptr_invoker<CallTargetCategory::Pointer> {
  template <class C,
            class Pointed,
            class Ty,
            class... Types,
            enable_if_t<is_function_v<Pointed>, int> = 0>
  static decltype(auto) invoke(
      Pointed C::*fn,
      Ty&& obj,
      Types&&... args) noexcept(noexcept(((*forward<Ty>(obj)).*
                                          fn)(forward<Types>(args)...))) {
    return ((*forward<Ty>(obj)).*fn)(forward<Types>(args)...);
  }

  template <
      class C,
      class Pointed,
      class Ty,
      class... Types,
      enable_if_t<!is_function_v<Pointed> && sizeof...(Types) == 0, int> = 0>
  static decltype(auto) invoke(
      Pointed C::*fn,
      Ty&& obj,
      Types&&... args) noexcept(noexcept((*forward<Ty>(obj)).*fn)) {
    return (*forward<Ty>(obj)).*fn;
  }
};

// template <class C, class Pointed, class Ty, class... Types>
// constexpr decltype(auto) invoke_by_member_ptr(Pointed C::*fn,
//                                              Ty&& obj,
//                                              Types&&... args) {
//  if constexpr (is_function_v<Pointed>) {
//    if constexpr (is_base_of_v<C, decay_t<Ty> >) {
//      return (forward<Ty>(obj).*fn)(forward<Types>(args)...);
//    } else if constexpr (is_reference_wrapper_v<decay_t<Ty> >) {
//      return (obj.get().*fn)(forward<Types>(args)...);
//    } else {
//      return ((*forward<Ty>(obj)).*fn)(forward<Types>(args)...);
//    }
//  } else {
//    static_assert(is_object_v<Pointed> && sizeof...(args) == 0);
//    if constexpr (is_base_of_v<C, decay_t<Ty> >) {
//      return forward<Ty>(obj).*fn;
//    } else if constexpr (is_reference_wrapper_v<decay_t<Ty> >) {
//      return obj.get().*fn;
//    } else {
//      return (*forward<Ty>(obj)).*fn;
//    }
//  }
//}

template <class FnTy, bool = is_member_pointer_v<decay_t<FnTy> > >
struct invoker {
  template <class C, class Pointed, class Ty, class... Types>
  static auto invoke(Pointed C::*fn, Ty&& obj, Types&&... args) noexcept(
      noexcept(member_ptr_invoker<get_call_target_category<C, Ty>()>::invoke(
          fn,
          forward<Ty>(obj),
          forward<Types>(args)...)))
      -> decltype(member_ptr_invoker<get_call_target_category<C, Ty>()>::invoke(
          fn,
          forward<Ty>(obj),
          forward<Types>(args)...)) {
    return member_ptr_invoker<get_call_target_category<C, Ty>()>::invoke(
        fn, forward<Ty>(obj), forward<Types>(args)...);
  }
};

template <class FnTy>
struct invoker<FnTy, false> {
  template <class Fn, class... Types>
  static auto invoke(Fn&& fn, Types&&... args) noexcept(
      noexcept(forward<Fn>(fn)(forward<Types>(args)...)))
      -> decltype(forward<Fn>(fn)(forward<Types>(args)...)) {
    return forward<Fn>(fn)(forward<Types>(args)...);
  }
};
}  // namespace fn::details
}  // namespace ktl
#endif