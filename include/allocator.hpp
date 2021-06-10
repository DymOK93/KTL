#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <memory>
namespace ktl {
template <class Ty>
using basic_allocator = std::allocator<Ty>;
using std::allocator_traits;
}  // namespace ktl
#else
#include <heap.h>
#include <new_delete.h>
#include <memory_tools.hpp>
#include <memory_type_traits.hpp>
#include <type_traits.hpp>

namespace ktl {
template <class Ty, align_val_t Alignment>
struct aligned_allocator_base {
 public:
  using value_type = Ty;
  using size_type = size_t;
  using differense_type = ptrdiff_t;
  using propagate_on_container_copy_assignment = true_type;
  using propagate_on_container_move_assignment = true_type;
  using propagate_on_container_swap = true_type;
  using is_always_equal = true_type;
  using enable_delete_null = true_type;

 public:
  static constexpr auto ALLOCATION_ALIGNMENT{Alignment};

 public:
  Ty* allocate(size_t object_count) {
    static_assert(always_false_v<Ty>,
                  "allocate() is non-virtual");  // static_assert(false, ...)
                                                 // сработает в независимости от
                                                 // наличия вызова allocate()
  }

  Ty* allocate_bytes(size_t bytes_count) {
    static_assert(
        always_false_v<Ty>,
        "allocate_bytes() is non-virtual");  // static_assert(false, ...)
                                             // сработает в независимости от
                                             // наличия вызова allocate()
  }

  void deallocate(Ty* ptr, size_t object_count) noexcept {
    operator delete(ptr, object_count * sizeof(Ty), ALLOCATION_ALIGNMENT);
  }

  void deallocate_bytes(Ty* ptr, size_t bytes_count) noexcept {
    operator delete(ptr, bytes_count, ALLOCATION_ALIGNMENT);
  }

 protected:
  template <typename NewTag>
  Ty* allocate_objects_impl(size_t object_count, NewTag new_tag) {
    return allocate_bytes_impl(object_count * sizeof(Ty), new_tag);
  }

  template <typename NewTag>
  Ty* allocate_bytes_impl(size_t bytes_count, NewTag new_tag) {
    return static_cast<Ty*>(operator new(bytes_count, ALLOCATION_ALIGNMENT,
                                         new_tag));
  }
};

template <class Ty, align_val_t Alignment>
constexpr bool operator==(
    const aligned_allocator_base<Ty, Alignment>&,
    const aligned_allocator_base<Ty, Alignment>&) noexcept {
  return true;
}

template <class Ty, align_val_t Alignment>
constexpr bool operator!=(
    const aligned_allocator_base<Ty, Alignment>&,
    const aligned_allocator_base<Ty, Alignment>&) noexcept {
  return false;
}

template <class Ty, align_val_t Alignment>
struct aligned_paged_allocator : aligned_allocator_base<Ty, Alignment> {
 public:
  using MyBase = aligned_allocator_base<Ty, Alignment>;
  using value_type = typename MyBase::value_type;
  using size_type = typename MyBase::size_type;
  using differense_type = typename MyBase::differense_type;
  using propagate_on_container_copy_assignment =
      typename MyBase::propagate_on_container_copy_assignment;
  using propagate_on_container_move_assignment =
      typename MyBase::propagate_on_container_move_assignment;
  using propagate_on_container_swap =
      typename MyBase::propagate_on_container_swap;
  using is_always_equal = typename MyBase::is_always_equal;
  using enable_delete_null = typename MyBase::enable_delete_null;

 public:
  template <class OtherTy>
  struct rebind {
    using other = aligned_paged_allocator<OtherTy, Alignment>;
  };

 public:
  Ty* allocate(size_t object_count) {
    return MyBase::allocate_objects_impl(object_count, paged_new);
  }

  Ty* allocate_bytes(size_t bytes_count) {
    return MyBase::allocate_bytes_impl(bytes_count, paged_new);
  }
};

template <class Ty, align_val_t Alignment>
struct aligned_non_paged_allocator : aligned_allocator_base<Ty, Alignment> {
 public:
  using MyBase = aligned_allocator_base<Ty, Alignment>;
  using value_type = typename MyBase::value_type;
  using size_type = typename MyBase::size_type;
  using differense_type = typename MyBase::differense_type;
  using propagate_on_container_copy_assignment =
      typename MyBase::propagate_on_container_copy_assignment;
  using propagate_on_container_move_assignment =
      typename MyBase::propagate_on_container_move_assignment;
  using propagate_on_container_swap =
      typename MyBase::propagate_on_container_swap;
  using is_always_equal = typename MyBase::is_always_equal;
  using enable_delete_null = typename MyBase::enable_delete_null;

 public:
  template <class OtherTy>
  struct rebind {
    using other = aligned_non_paged_allocator<OtherTy, Alignment>;
  };

 public:
  Ty* allocate(size_t object_count) {
    return MyBase::allocate_objects_impl(object_count, non_paged_new);
  }

  Ty* allocate_bytes(size_t bytes_count) {
    return MyBase::allocate_bytes_impl(bytes_count, non_paged_new);
  }
};

template <class Ty>
using basic_paged_allocator =
    aligned_paged_allocator<Ty, crt::DEFAULT_ALLOCATION_ALIGNMENT>;

template <class Ty>
using basic_non_paged_allocator =
    aligned_non_paged_allocator<Ty, crt::DEFAULT_ALLOCATION_ALIGNMENT>;

template <class Alloc>
struct allocator_traits {
 public:
  using allocator_type = Alloc;
  using value_type = typename Alloc::value_type;
  using pointer = mm::details::get_pointer_type_t<Alloc, value_type>;
  using const_pointer =
      mm::details::get_const_pointer_type_t<Alloc, value_type>;
  using reference = mm::details::get_reference_type_t<Alloc, value_type>;
  using const_reference = add_const_t<reference>;
  using size_type = mm::details::get_size_type_t<Alloc>;
  using difference_type = mm::details::get_difference_type<Alloc>;

  using propagate_on_container_copy_assignment =
      mm::details::get_propagate_on_container_copy_assignment_t<Alloc>;
  using propagate_on_container_move_assignment =
      mm::details::get_propagate_on_container_move_assignment_t<Alloc>;
  using propagate_on_container_swap =
      mm::details::get_propagate_on_container_swap_t<Alloc>;
  using is_always_equal = mm::details::get_always_equal_t<Alloc>;
  using enable_delete_null = mm::details::get_enable_delete_null<Alloc>;

 private:
  template <class OtherTy>
  struct get_rebind_alloc_type {
    using type = typename Alloc::template rebind<OtherTy>::other;
  };
  template <class OtherTy>
  struct get_rebind_traits_type {
    using other = typename allocator_traits<
        typename get_rebind_alloc_type<OtherTy>::type>;
  };

 public:
  template <class OtherTy>
  using rebind_alloc = typename get_rebind_alloc_type<OtherTy>::type;

  template <class OtherTy>
  using rebind_traits = typename get_rebind_traits_type<OtherTy>::type;

 public:
  static constexpr allocator_type select_on_container_copy_construction(
      const allocator_type& alloc) {
    if constexpr (mm::details::has_select_on_container_copy_construction_v<
                      allocator_type>) {
      return alloc.select_on_container_copy_construction();
    } else {
      return alloc;
    }
  }

  static constexpr pointer allocate(
      allocator_type& alloc,
      size_type object_count) noexcept(noexcept(alloc.allocate(object_count))) {
    static_assert(mm::details::has_allocate_v<allocator_type, size_type>,
                  "Allocator must provide allocate(size_type) function");
    return alloc.allocate(object_count);
  }

  static constexpr pointer allocate_single_object(
      allocator_type& alloc) noexcept(noexcept(alloc.allocate())) {
    static_assert(mm::details::has_allocate_single_object_v<allocator_type>,
                  "Allocator must provide allocate(void) function");
    return alloc.allocate();
  }

  static constexpr pointer allocate_bytes(
      allocator_type& alloc,
      size_type bytes_count) noexcept(noexcept(alloc.allocate(bytes_count))) {
    static_assert(mm::details::has_allocate_bytes_v<allocator_type, size_type>,
                  "Allocator must provide allocate_bytes(size_type) function");
    return alloc.allocate_bytes(bytes_count);
  }

  static constexpr void deallocate(
      allocator_type& alloc,
      pointer ptr,
      size_type
          object_count) noexcept(noexcept(alloc.deallocate(ptr,
                                                           object_count))) {
    static_assert(
        mm::details::has_deallocate_v<allocator_type, pointer, size_type>,
        "Allocator must provide deallocate(pointer, size_type) function");
    alloc.deallocate(ptr, object_count);
  }

  static constexpr void deallocate_single_object(
      allocator_type& alloc,
      pointer ptr) noexcept(noexcept(alloc.deallocate(ptr))) {
    static_assert(
        mm::details::has_deallocate_single_object_v<allocator_type, pointer>,
        "Allocator must provide deallocate(pointer) function");
    alloc.deallocate(ptr);
  }

  static constexpr void deallocate_bytes(
      allocator_type& alloc,
      void* ptr,
      size_type bytes_count) noexcept(noexcept(alloc
                                                   .deallocate(
                                                       static_cast<pointer>(
                                                           ptr),
                                                       bytes_count))) {
    static_assert(
        mm::details::has_deallocate_bytes_v<allocator_type, pointer, size_type>,
        "Allocator must provide deallocate_bytes(pointer ptr, size_type "
        "bytes_count) function");
    alloc.deallocate_bytes(static_cast<pointer>(ptr), bytes_count);
  }

  template <class... Types>
  static constexpr pointer
  construct(allocator_type& alloc, pointer ptr, Types&&... args) noexcept(
      is_nothrow_constructible_v<value_type, Types...>) {
    if constexpr (mm::details::has_construct_v<allocator_type, pointer,
                                               Types...>) {
      return alloc_construct(alloc, ptr, forward<Types>(args)...);
    } else {
      (void)alloc;
      return in_place_construct(ptr, forward<Types>(args)...);
    }
  }

  static constexpr void destroy(allocator_type& alloc,
                                pointer ptr) {  // TODO: conditional noexcept
    if constexpr (mm::details::has_destroy_v<allocator_type, pointer>) {
      alloc_destroy(alloc, ptr);
    } else {
      in_place_destroy(ptr);
    }
  }

 private:
  template <class... Types>
  static pointer alloc_construct(allocator_type& alloc,
                                 pointer ptr,
                                 Types&&... args) {
    return alloc.construct(ptr, forward<Types>(args)...);
  }

  template <class... Types>
  static pointer in_place_construct(pointer ptr, Types&&... args) {
    return construct_at(ptr, forward<Types>(args)...);
  }

  static void alloc_destroy(allocator_type& alloc, pointer ptr) {
    alloc.destroy(ptr);
  }

  static void in_place_destroy(pointer ptr) { destroy_at(ptr); }
};
}  // namespace ktl
#endif
