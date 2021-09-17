#pragma once
#include <functional_impl.hpp>
#include <type_traits_impl.hpp>

namespace ktl {
template <class Ty>
struct alignment_of
    : integral_constant<size_t,
                        alignof(typename remove_all_extents<Ty>::type)> {};

template <class T, T... Ints>
class integer_sequence {
 public:
  using value_type = T;
  static_assert(is_integral<value_type>::value, "not integral type");
  static constexpr size_t size() noexcept { return sizeof...(Ints); }
};
template <size_t... Inds>
using index_sequence = integer_sequence<size_t, Inds...>;

namespace tt::details {
template <class Ty, Ty Begin, Ty End, bool>
struct integer_sequence_impl {
  using value_type = Ty;
  static_assert(is_integral_v<value_type>, "not integral type");
  static_assert(Begin >= 0 && Begin < End,
                "unexpected argument (Begin<0 || Begin<=End)");

  template <class, class>
  struct integer_sequence_combiner;

  template <value_type... Inds0, value_type... Inds1>
  struct integer_sequence_combiner<integer_sequence<value_type, Inds0...>,
                                   integer_sequence<value_type, Inds1...>> {
    using result_type = integer_sequence<value_type, Inds0..., Inds1...>;
  };

  using result_type = typename integer_sequence_combiner<
      typename integer_sequence_impl<value_type,
                                     Begin,
                                     Begin + (End - Begin) / 2,
                                     (End - Begin) / 2 == 1>::result_type,
      typename integer_sequence_impl<value_type,
                                     Begin + (End - Begin) / 2,
                                     End,
                                     (End - Begin + 1) / 2 ==
                                         1>::result_type>::result_type;
};

template <class Ty, Ty Begin>
struct integer_sequence_impl<Ty, Begin, Begin, false> {
  using value_type = Ty;
  static_assert(is_integral_v<value_type>, "not integral type");
  static_assert(Begin >= 0, "unexpected argument (Begin<0)");
  using result_type = integer_sequence<value_type>;
};

template <class Ty, Ty Begin, Ty End>
struct integer_sequence_impl<Ty, Begin, End, true> {
  using value_type = Ty;
  static_assert(is_integral_v<value_type>, "not integral type");
  static_assert(Begin >= 0, "unexpected argument (Begin<0)");
  using result_type = integer_sequence<value_type, Begin>;
};
}  // namespace tt::details

template <class Ty, Ty N>
using make_integer_sequence = typename tt::details::
    integer_sequence_impl<Ty, 0, N, (N - 0) == 1>::result_type;

template <size_t N>
using make_index_sequence = make_integer_sequence<size_t, N>;

template <class... Types>
using index_sequence_for = make_index_sequence<sizeof...(
    Types)>;  // sizeof...(arg_pack) вычисляет количество
              // аргументов variadic-шаблона

template <class Ty>
struct identity {
  using type = Ty;
};

template <class Ty>
using identity_t = typename identity<Ty>::type;

template <class IntegralTy>
struct make_unsigned;

template <>
struct make_unsigned<char> {
  using type = unsigned char;
};

template <>
struct make_unsigned<short> {
  using type = unsigned short;
};

template <>
struct make_unsigned<int> {
  using type = unsigned int;
};

template <>
struct make_unsigned<long> {
  using type = unsigned long;
};

template <>
struct make_unsigned<long long> {
  using type = unsigned long long;
};

template <class IntegralTy>
using make_unsigned_t = typename make_unsigned<IntegralTy>::type;

template <typename Ty>
inline constexpr bool is_memcpyable_v =
    is_trivially_copyable_v<Ty>&& is_trivially_destructible_v<Ty>;

template <typename Ty>
struct is_memcpyable : bool_constant<is_memcpyable_v<Ty>> {};

template <class InputIt, class OutputIt>
inline constexpr bool is_memcpyable_range_v = false;

template <typename Ty>
inline constexpr bool is_memcpyable_range_v<Ty*, Ty*> = is_memcpyable_v<Ty>;

template <class InputIt, class OutputIt>
struct is_memcpyable_range
    : bool_constant<is_memcpyable_range_v<InputIt, OutputIt>> {};

#define INVOKE_EXPR \
  fn::details::invoker<Fn>::invoke(declval<Fn>(), declval<Types>()...)

template <class, class Fn, class... Types>
struct invoke_result {};

template <class Fn, class... Types>
struct invoke_result<void_t<decltype(INVOKE_EXPR)>, Fn, Types...> {
  using type = decltype(INVOKE_EXPR);
};

template <class Fn, class... Types>
using invoke_result_t = typename invoke_result<void_t<>, Fn, Types...>::type;

template <class, class Fn, class... Types>
struct is_invocable : false_type {};

template <class Fn, class... Types>
struct is_invocable<void_t<invoke_result_t<Fn, Types...>>, Fn, Types...>
    : true_type {};

template <class Fn, class... Types>
inline constexpr bool is_invocable_v =
    is_invocable<void_t<>, Fn, Types...>::value;

template <class, class Ret, class Fn, class... Types>
struct is_invocable_r : false_type {};

template <class Ret, class Fn, class... Types>
struct is_invocable_r<enable_if_t<is_invocable_v<Fn, Types...>>,
                      Ret,
                      Fn,
                      Types...>
    : bool_constant<is_void_v<Ret> ||
                    is_convertible_v<invoke_result_t<Fn, Types...>, Ret>> {};

template <class Ret, class Fn, class... Types>
inline constexpr bool is_invocable_r_v =
    is_invocable_r<void_t<>, Ret, Fn, Types...>::value;

template <class, class Fn, class... Types>
struct is_nothrow_invocable : false_type {};

template <class Fn, class... Types>
struct is_nothrow_invocable<enable_if_t<is_invocable_v<Fn, Types...>>,
                            Fn,
                            Types...> : bool_constant<noexcept(INVOKE_EXPR)> {};

template <class Fn, class... Types>
inline constexpr bool is_nothrow_invocable_v =
    is_nothrow_invocable<void_t<>, Fn, Types...>::value;

template <class, class Ret, class Fn, class... Types>
struct is_nothrow_invocable_r : false_type {};

template <class Ret, class Fn, class... Types>
struct is_nothrow_invocable_r<enable_if_t<is_invocable_r_v<Fn, Types...>>,
                              Ret,
                              Fn,
                              Types...> : bool_constant<noexcept(INVOKE_EXPR)> {
};

template <class Ret, class Fn, class... Types>
inline constexpr bool is_nothrow_invocable_r_v =
    is_nothrow_invocable_r<void_t<>, Ret, Fn, Types...>::value;

#undef INVOKE_EXPR
}  // namespace ktl
