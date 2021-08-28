#pragma once
#include <basic_types.hpp>
#include <type_traits_impl.hpp>

namespace ktl::mm::details {
template <class Target, class Ty, class = void>
struct get_pointer_type {
  using type = add_pointer_t<Ty>;
};

template <class Target, class Ty>
struct get_pointer_type<Target, Ty, void_t<typename Target::pointer>> {
  using type = typename Target::pointer;
};

template <class Target, class Ty>
using get_pointer_type_t = typename get_pointer_type<Target, Ty>::type;

template <class Target, class Ty, class = void>
struct get_const_pointer_type {
  using type = add_pointer_t<add_const_t<Ty>>;
};

template <class Target, class Ty>
struct get_const_pointer_type<Target,
                              Ty,
                              void_t<typename Target::const_pointer>> {
  using type = typename Target::pointer;
};

template <class Target, class Ty>
using get_const_pointer_type_t =
    typename get_const_pointer_type<Target, Ty>::type;

template <class Target, class Ty, class = void>
struct get_reference_type {
  using type = add_lvalue_reference_t<Ty>;
};

template <class Target, class Ty>
struct get_reference_type<Target, Ty, void_t<typename Target::reference>> {
  using type = typename Target::reference;
};

template <class Target, class Ty>
using get_reference_type_t = typename get_reference_type<Target, Ty>::type;

template <class Target, class Ty, class = void>
struct get_const_reference_type {
  using type = add_lvalue_reference_t<add_const_t<Ty>>;
};

template <class Target, class Ty>
struct get_const_reference_type<Target,
                                Ty,
                                void_t<typename Target::const_reference>> {
  using type = typename Target::const_reference;
};

template <class Target, class Ty>
using get_const_reference_type_t =
    typename get_const_reference_type<Target, Ty>::type;

template <class Target, class = void>
struct get_size_type {
  using type = size_t;
};

template <class Target>
struct get_size_type<Target, void_t<typename Target::size_type>> {
  using type = typename Target::size_type;
};

template <class Target>
using get_size_type_t = typename get_size_type<Target>::type;

template <class Target, class = void>
struct get_difference_type {
  using type = ptrdiff_t;
};

template <class Target>
struct get_difference_type<Target, void_t<typename Target::difference_type>> {
  using type = typename Target::difference_type;
};

template <class Target>
using get_difference_type_t = typename get_difference_type<Target>::type;

template <class Target, class = void>
struct get_propagate_on_container_move_assignment {
  using type = false_type;
};

template <class Target>
struct get_propagate_on_container_move_assignment<
    Target,
    void_t<typename Target::propagate_on_container_move_assignment>> {
  using type = typename Target::propagate_on_container_move_assignment;
};

template <class Target>
using get_propagate_on_container_move_assignment_t =
    typename get_propagate_on_container_move_assignment<Target>::type;

template <class Target, class = void>
struct get_propagate_on_container_copy_assignment {
  using type = false_type;
};

template <class Target>
struct get_propagate_on_container_copy_assignment<
    Target,
    void_t<typename Target::propagate_on_container_copy_assignment>> {
  using type = typename Target::propagate_on_container_copy_assignment;
};

template <class Target>
using get_propagate_on_container_copy_assignment_t =
    typename get_propagate_on_container_copy_assignment<Target>::type;

template <class Target, class = void>
struct get_propagate_on_container_swap {
  using type = false_type;
};

template <class Target>
struct get_propagate_on_container_swap<
    Target,
    void_t<typename Target::propagate_on_container_swap>> {
  using type = typename Target::propagate_on_container_swap;
};

template <class Target>
using get_propagate_on_container_swap_t =
    typename get_propagate_on_container_swap<Target>::type;

template <class Target, bool empty>
struct get_always_equal_helper : false_type {};

template <class Target>
struct get_always_equal_helper<Target, true> : true_type {};

template <class Target, bool empty>
using get_always_equal_helper_t =
    typename get_always_equal_helper<Target, empty>::type;

template <class Target, class = void>
struct get_always_equal {
  using type = get_always_equal_helper_t<Target, is_empty_v<Target>>;
};

template <class Target>
struct get_always_equal<Target, void_t<typename Target::is_always_equal>> {
  using type = typename Target::is_always_equal;
};

template <class Target>
using get_always_equal_t = typename get_always_equal<Target>::type;

template <class Alloc, class size_type, class = void>
struct has_allocate : false_type {};

template <class Alloc, class size_type>
struct has_allocate<
    Alloc,
    size_type,
    void_t<decltype(declval<Alloc>().allocate(declval<size_type>()))>>
    : true_type {};

template <class Alloc, class size_type>
inline constexpr bool has_allocate_v = has_allocate<Alloc, size_type>::value;

template <class Alloc, class = void>
struct has_allocate_single_object : false_type {};

template <class Alloc>
struct has_allocate_single_object<Alloc,
                                  void_t<decltype(declval<Alloc>().allocate())>>
    : true_type {};

template <class Alloc>
inline constexpr bool has_allocate_single_object_v =
    has_allocate_single_object<Alloc>::value;

template <class Alloc, class size_type, class = void>
struct has_allocate_bytes : false_type {};

template <class Alloc, class size_type>
struct has_allocate_bytes<
    Alloc,
    size_type,
    void_t<decltype(declval<Alloc>().allocate_bytes(declval<size_type>()))>>
    : true_type {};

template <class Alloc, class size_type>
inline constexpr bool has_allocate_bytes_v =
    has_allocate_bytes<Alloc, size_type>::value;

template <class Alloc, class Pointer, class size_type, class = void>
struct has_deallocate : false_type {};

template <class Alloc, class Pointer, class size_type>
struct has_deallocate<
    Alloc,
    Pointer,
    size_type,
    void_t<decltype(
        declval<Alloc>().deallocate(declval<Pointer>(), declval<size_type>()))>>
    : true_type {};

template <class Alloc, class Pointer, class size_type>
inline constexpr bool has_deallocate_v =
    has_deallocate<Alloc, Pointer, size_type>::value;

template <class Alloc, class Pointer, class = void>
struct has_deallocate_single_object : false_type {};

template <class Alloc, class Pointer>
struct has_deallocate_single_object<
    Alloc,
    Pointer,
    void_t<decltype(declval<Alloc>().deallocate(declval<Pointer>()))>>
    : true_type {};

template <class Alloc, class Pointer>
inline constexpr bool has_deallocate_single_object_v =
    has_deallocate_single_object<Alloc, Pointer>::value;

template <class Alloc, class Pointer, class size_type, class = void>
struct has_deallocate_bytes : false_type {};

template <class Alloc, class Pointer, class size_type>
struct has_deallocate_bytes<
    Alloc,
    Pointer,
    size_type,
    void_t<decltype(declval<Alloc>().deallocate_bytes(declval<Pointer>(),
                                                      declval<size_type>()))>>
    : true_type {};

template <class Alloc, class Pointer, class size_type>
inline constexpr bool has_deallocate_bytes_v =
    has_deallocate_bytes<Alloc, Pointer, size_type>::value;

template <class, class Alloc, class Pointer, class... Types>
struct has_construct : false_type {};

template <class Alloc, class Pointer, class... Types>
struct has_construct<
    void_t<decltype(
        declval<Alloc>().construct(declval<Pointer>(), declval<Types>()...))>,
    Alloc,
    Pointer,
    Types...> : true_type {};

template <class Alloc, class Pointer, class... Types>
inline constexpr bool has_construct_v =
    has_construct<void_t<>, Alloc, Pointer, Types...>::value;

template <class Alloc, class Pointer, class = void>
struct has_destroy : false_type {};

template <class Alloc, class Pointer>
struct has_destroy<
    Alloc,
    Pointer,
    void_t<decltype(declval<Alloc>().destroy(declval<Pointer>()))>>
    : true_type {};

template <class Alloc, class Pointer>
inline constexpr bool has_destroy_v = has_destroy<Alloc, Pointer>::value;

template <class Alloc, class = void>
struct has_max_size : false_type {};

template <class Alloc>
struct has_max_size<Alloc, void_t<decltype(declval<Alloc>().max_size())>>
    : true_type {};

template <class Alloc>
inline constexpr bool has_max_size_v = has_max_size<Alloc>::value;

template <class Alloc, class = void>
struct has_select_on_container_copy_construction : false_type {};

template <class Alloc>
struct has_select_on_container_copy_construction<
    Alloc,
    void_t<decltype(declval<Alloc>().select_on_container_copy_construction())>>
    : true_type {};

template <class Alloc>
inline constexpr bool has_select_on_container_copy_construction_v =
    has_select_on_container_copy_construction<Alloc>::value;

template <class Deleter, class = void>
struct get_enable_delete_null : false_type {};

template <class Deleter>
struct get_enable_delete_null<
    Deleter,
    void_t<decltype(Deleter::enable_delete_null::value)>> {
  static constexpr bool value = Deleter::enable_delete_null::value;
};

template <class Deleter>
inline constexpr bool get_enable_delete_null_v =
    get_enable_delete_null<Deleter>::value;

template <class Ty>
struct memset_is_safe : false_type {};

template <>
struct memset_is_safe<char> : true_type {};

template <>
struct memset_is_safe<signed char> : true_type {};

template <>
struct memset_is_safe<unsigned char> : true_type {};

template <>
struct memset_is_safe<bool> : true_type {};

template <class Ty>
inline constexpr bool memset_is_safe_v = memset_is_safe<Ty>::value;

template <class Ty>
struct wmemset_is_safe : false_type {};

template <>
struct wmemset_is_safe<wchar_t> : true_type {};

template <class Ty>
inline constexpr bool wmemset_is_safe_v = wmemset_is_safe<Ty>::value;
}  // namespace ktl::mm::details