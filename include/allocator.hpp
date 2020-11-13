#pragma once

#ifndef NO_CXX_STANDARD_LIBRARY
#include <memory>
namespace winapi::kernel::mm {
template <class Ty>
using basic_allocator = std::allocator<Ty>;

using std::allocator_traits;
}  // namespace winapi::kernel::mm
#else
#include <heap.h>
#include <memory_tools.hpp>
#include <memory_type_traits.hpp>

namespace winapi::kernel::mm {
template <class Ty>
struct basic_allocator {
 public:
  using value_type = Ty;
  using size_type = size_t;
  using differense_type = ptrdiff_t;
  using propagate_on_container_move_assignment = true_type;
  using is_always_equal = true_type;

 public:
  Ty* allocate(size_t bytes_count) {
    static_assert(always_false_v<>,
                  "allocate() is non-virtual");  // static_assert(false, ...)
                                                 // срабоает в независимости от
                                                 // наличия вызова allocate()
  }
  void deallocate(Ty* ptr, size_t bytes_count) { operator delete(ptr); }

 protected:
  template <typename NewTag>
  Ty* allocate_helper(size_t bytes_count, NewTag new_tag) {
    return static_cast<Ty*>(operator new(bytes_count, nothrow, new_tag));
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

 public:
  template <class OtherTy>
  struct rebind {
    using other = basic_paged_allocator<OtherTy>;
  };

 public:
  Ty* allocate(size_t bytes_count) {
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

 public:
  template <class OtherTy>
  struct rebind {
    using other = basic_allocator<OtherTy>;
  };

 public:
  Ty* allocate(size_t bytes_count) {
    return MyBase::allocate_helper(bytes_count, non_paged_new);
  }
};

template <class Alloc>
struct allocator_traits {
 public:
  using allocator_type = Alloc;
  using value_type = typename Alloc::value_type;
  using pointer = details::get_pointer_type_t<Alloc, value_type>;
  using const_pointer = add_const_t<pointer>;
  using size_type = details::get_size_type_t<Alloc>;
  using difference_type = details::get_difference_type<Alloc>;

  using propagate_on_container_copy_assignment =
      details::get_propagate_on_container_copy_assignment_t<Alloc>;
  using propagate_on_container_move_assignment =
      details::get_propagate_on_container_move_assignment_t<Alloc>;
  using propagate_on_container_swap =
      details::get_propagate_on_container_swap_t<Alloc>;
  using is_always_equal = details::get_always_equal_t<Alloc>;

 public:
  template <class OtherTy>
  struct rebind_alloc {
    using other = typename Alloc::template rebind<OtherTy>::other;
  };
  template <class OtherTy>
  struct rebind_traits {
    using other = allocator_traits<typename rebind_alloc<OtherTy>::other>;
  };

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
    static_assert(details::has_allocate_v<Alloc, size_type>,
                  "Allocator must provide allocate(size_type) function");
    return alloc.allocate(bytes_count);
  }

  static constexpr void deallocate(Alloc& alloc,
                                   pointer ptr,
                                   size_type object_count) {
    static_assert(
        details::has_deallocate_v<Alloc, pointer, size_type>,
        "Allocator must provide deallocate(pointer, size_type) function");
    alloc.deallocate(ptr, object_count * sizeof(value_type));
  }

  static constexpr void deallocate_single_object(Alloc& alloc, pointer ptr) {
    static_assert(details::has_deallocate_v<Alloc, pointer, size_type>,
                  "Allocator must provide deallocate(pointer) function");
    alloc.deallocate(ptr);
  }

  static constexpr void deallocate_bytes(Alloc& alloc,
                                         void* ptr,
                                         size_type bytes_count) {
    static_assert(details::has_deallocate_v<Alloc, pointer, size_type>,
                  "Allocator must provide deallocate(pointer ptr, size_type "
                  "bytes_count) function");
    alloc.deallocate(static_cast<pointer>(ptr), bytes_count);
  }

  template <class... Types>
  static constexpr pointer construct(Alloc& alloc,
                                     pointer ptr,
                                     Types&&... args) {
    if constexpr (details::has_construct_v<Alloc, pointer, Types...>) {
      return alloc_construct(alloc, ptr, forward<Types>(args)...);
    } else {
      return in_place_construct(ptr, forward<Types>(args)...);
    }
  }

  static constexpr void destroy(Alloc& alloc, pointer ptr) {
    if constexpr (details::has_destroy_v<Alloc, pointer>) {
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
}  // namespace winapi::kernel::mm
#endif
