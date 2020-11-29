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
#include <memory_tools.hpp>
#include <memory_type_traits.hpp>

namespace ktl {
template <class Ty>
struct basic_allocator {
 public:
  using value_type = Ty;
  using size_type = size_t;
  using differense_type = ptrdiff_t;
  using propagate_on_container_move_assignment = true_type;
  using is_always_equal = true_type;
  using enable_delete_null = true_type;

 public:
  Ty* allocate(size_t object_count) {
    static_assert(always_false_v<Ty>,
                  "allocate() is non-virtual");  // static_assert(false, ...)
                                                 // сработает в независимости от
                                                 // наличия вызова allocate()
  }

  Ty* allocate_bytes(size_t bytes_count) {
    static_assert(always_false_v<Ty>,
                  "allocate_bytes() is non-virtual");   // static_assert(false, ...)
                                                        // сработает в независимости от
                                                        // наличия вызова allocate()
  }

  void deallocate(Ty* ptr, size_t object_count) {
    operator delete(ptr, object_count * sizeof(Ty));
  }

  void deallocate_bytes(Ty* ptr, size_t bytes_count) {
    operator delete(ptr, bytes_count);
  }

 protected:
  template <typename NewTag>
  Ty* allocate_helper(size_t object_count, NewTag new_tag) {
    return static_cast<Ty*>(operator new(object_count * sizeof(Ty), nothrow,
                                         new_tag));
  }
};

template <class Ty>
struct basic_paged_allocator : basic_allocator<Ty> {
 public:
  using MyBase = basic_allocator<Ty>;
  using value_type = typename MyBase::value_type;
  using size_type = typename MyBase::size_type;
  using differense_type = typename MyBase::differense_type;
  using propagate_on_container_move_assignment =
      typename MyBase::propagate_on_container_move_assignment;
  using is_always_equal = typename MyBase::is_always_equal;
  using enable_delete_null = typename MyBase::enable_delete_null;

 public:
  template <class OtherTy>
  struct rebind {
    using other = basic_paged_allocator<OtherTy>;
  };

 public:
  Ty* allocate(size_t object_count) {
    return MyBase::allocate_helper(object_count, paged_new);
  }

  Ty* allocate_bytes(size_t bytes_count) {
    return MyBase::allocate_helper(bytes_count, paged_new);
  }
};

template <class Ty>
struct basic_non_paged_allocator : basic_allocator<Ty> {
 public:
  using MyBase = basic_allocator<Ty>;
  using value_type = typename MyBase::value_type;
  using size_type = typename MyBase::size_type;
  using differense_type = typename MyBase::differense_type;
  using propagate_on_container_move_assignment =
      typename MyBase::propagate_on_container_move_assignment;
  using is_always_equal = typename MyBase::is_always_equal;
  using enable_delete_null = typename MyBase::enable_delete_null;

 public:
  template <class OtherTy>
  struct rebind {
    using other = basic_allocator<OtherTy>;
  };

 public:
  Ty* allocate(size_t object_count) {
    return MyBase::allocate_helper(object_count, non_paged_new);
  }

  Ty* allocate_bytes(size_t bytes_count) {
    return MyBase::allocate_helper(bytes_count, non_paged_new);
  }
};

template <class Alloc>
struct allocator_traits {
 public:
  using allocator_type = Alloc;
  using value_type = typename Alloc::value_type;
  using pointer = mm::details::get_pointer_type_t<Alloc, value_type>;
  using const_pointer = add_const_t<pointer>;
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
  static constexpr pointer allocate(Alloc& alloc, size_type object_count) {
    static_assert(details::has_allocate_v<Alloc, size_type>,
                  "Allocator must provide allocate(size_type) function");
    return alloc.allocate(object_count * sizeof(value_type));
  }

  static constexpr pointer allocate_single_object(Alloc& alloc) {
    static_assert(details::has_allocate_single_object_v<Alloc, size_type>,
                  "Allocator must provide allocate(void) function");
    return alloc.allocate();
  }

  static constexpr void* allocate_bytes(Alloc& alloc, size_type bytes_count) {
    static_assert(mm::details::has_allocate_bytes_v<Alloc, size_type>,
                  "Allocator must provide allocate(size_type) function");
    return alloc.allocate(bytes_count);
  }

  static constexpr void deallocate(Alloc& alloc,
                                   pointer ptr,
                                   size_type object_count) {
    static_assert(
        mm::details::has_deallocate_v<Alloc, pointer, size_type>,
        "Allocator must provide deallocate(pointer, size_type) function");
    alloc.deallocate(ptr, object_count * sizeof(value_type));
  }

  static constexpr void deallocate_single_object(Alloc& alloc, pointer ptr) {
    static_assert(
        mm::details::has_deallocate_single_object_v<Alloc, pointer>,
                  "Allocator must provide deallocate(pointer) function");
    alloc.deallocate(ptr);
  }

  static constexpr void deallocate_bytes(Alloc& alloc,
                                         void* ptr,
                                         size_type bytes_count) {
    static_assert(mm::details::has_deallocate_bytes_v<Alloc, pointer, size_type>,
                  "Allocator must provide deallocate(pointer ptr, size_type "
                  "bytes_count) function");
    alloc.deallocate(static_cast<pointer>(ptr), bytes_count);
  }

  template <class... Types>
  static constexpr pointer construct(Alloc& alloc,
                                     pointer ptr,
                                     Types&&... args) {
    if constexpr (mm::details::has_construct_v<Alloc, pointer, Types...>) {
      return alloc_construct(alloc, ptr, forward<Types>(args)...);
    } else {
      (void)alloc;
      return in_place_construct(ptr, forward<Types>(args)...);
    }
  }

  static constexpr void destroy(Alloc& alloc, pointer ptr) {
    if constexpr (mm::details::has_destroy_v<Alloc, pointer>) {
      alloc_destroy(alloc, ptr);
    } else {
      in_place_destroy(ptr);
    }
  }

 private:
  template <class... Types>
  static pointer alloc_construct(Alloc& alloc, pointer ptr, Types&&... args) {
    return alloc.construct(ptr, forward<Types>(args)...);
  }

  template <class... Types>
  static pointer in_place_construct(pointer ptr, Types&&... args) {
    return construct_at(ptr, forward<Types>(args)...);
  }

  static void alloc_destroy(Alloc& alloc, pointer ptr) { alloc.destroy(ptr); }

  static void in_place_destroy(pointer ptr) { destroy_at(ptr); }
};
}  // namespace ktl
#endif
