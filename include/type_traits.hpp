#pragma once
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

template <template <typename> class Trait, class... Types>
struct is_all {
  static constexpr bool value = (Trait<Types>::value && ...);
};

template <template <typename> class Trait, class... Types>
inline constexpr bool is_all_v = is_all<Trait, Types...>::value;

template <bool Value>
struct bool_tag {
  using type = true_type;
};

template <>
struct bool_tag<false> {
  using type = false_type;
};

template <bool Value>
using bool_tag_t = typename bool_tag<Value>::type;

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

struct non_copyable {
  non_copyable() = default;
  non_copyable(const non_copyable&) = delete;
  non_copyable& operator=(const non_copyable&) = delete;
  non_copyable(non_copyable&&) = default;
  non_copyable& operator=(non_copyable&&) = default;
};

struct non_relocatable {
  non_relocatable() = default;
  non_relocatable(const non_relocatable&) = delete;
  non_relocatable& operator=(const non_relocatable&) = delete;
  non_relocatable(non_relocatable&&) = delete;
  non_relocatable& operator=(non_relocatable&&) = delete;
};
}  // namespace ktl
