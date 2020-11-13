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
    return static_cast<Ty*>(operator new(bytes_count, nothrow));
  }
  void deallocate(Ty* ptr, size_t bytes_count) { operator delete(ptr); }

 public:
  template <class OtherTy>
  struct rebind {
    using other = basic_allocator<OtherTy>;
  };
};

template <class Alloc>
struct allocator_traits {
 public:
  using allocator_type = Alloc;
  using value_type = typename Alloc::value_type;
  using pointer = details::get_pointer_type_t<Alloc, value_type>;
  using const_pointer = add_const_t<pointer>;
  using size_type = details::get_size_type<Alloc>;
  using difference_type = details::get_difference_type<Alloc>;

  using propagate_on_container_copy_assignment =
      details::get_propagate_on_container_copy_assignment_t<Alloc>;
  using propagate_on_container_move_assignment =
      details::get_propagate_on_container_move_assignment_t<Alloc>;
  using propagate_on_container_swap =
      details::get_propagate_on_container_swap_t<Alloc>;
  using is_always_equal = details::get_always_equal_t<Alloc>;

 public:
  static constexpr pointer allocate(Alloc& alloc, size_type object_count) {
    static_assert(
        details::has_allocate_v<Alloc, size_type>,
        "Allocator must provide allocate(size_type bytes_count) function");
    return alloc.allocate(object_count * sizeof(value_type));
  }

  static constexpr pointer allocate_single_object(Alloc& alloc) {
    static_assert(details::has_allocate_single_object_v<Alloc, size_type>,
                  "Allocator must provide allocate(void) function");
    return alloc.allocate();
  }

  static constexpr pointer allocate_bytes(Alloc& alloc, size_type bytes_count) {
    static_assert(
        details::has_allocate_v<Alloc, size_type>,
        "Allocator must provide allocate(size_type bytes_count) function");
    return alloc.allocate(bytes_count);
  }

  static constexpr void deallocate(Alloc& alloc,
                                   pointer ptr,
                                   size_type object_count) {
    static_assert(details::has_deallocate_v<Alloc, pointer, size_type>,
                  "Allocator must provide deallocate(pointer ptr, size_type "
                  "bytes_count) function");
    alloc.deallocate(ptr, object_count * sizeof(value_type));
  }

  static constexpr void deallocate_single_object(Alloc& alloc, pointer ptr) {
    static_assert(details::has_deallocate_v<Alloc, pointer, size_type>,
                  "Allocator must provide deallocate(pointer ptr) function");
    alloc.deallocate(ptr);
  }

  static constexpr void deallocate_bytes(Alloc& alloc,
                                         pointer ptr,
                                         size_type bytes_count) {
    static_assert(details::has_deallocate_v<Alloc, pointer, size_type>,
                  "Allocator must provide deallocate(pointer ptr, size_type "
                  "bytes_count) function");
    alloc.deallocate(ptr, bytes_count);
  }

  template <class... Types>
  static constexpr pointer construct(Alloc& alloc,
                                     pointer ptr,
                                     Types&&... args) {
    if constexpr (details::has_construct_v<Alloc, pointer, Types...>) {
      return alloc.construct(ptr, forward<Types>(args)...);
    } else {
      return construct_at(ptr, forward<Types>(args)...);
    }
  }

  static constexpr void destroy(Alloc& alloc, pointer ptr) {
    if constexpr (details::has_destroy_v<Alloc, pointer>) {
      alloc.destroy(ptr);
    } else {
      destroy_at(ptr);
    }
  }
};
}  // namespace winapi::kernel::mm
#endif