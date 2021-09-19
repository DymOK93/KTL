#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <memory>
namespace ktl {
template <class Ty>
using basic_allocator = std::allocator<Ty>;
using std::allocator_traits;
}  // namespace ktl
#else
#include <heap.hpp>
#include <memory_impl.hpp>
#include <memory_type_traits.hpp>
#include <new_delete.hpp>

namespace ktl {
template <class Ty, class NewTag, ::std::align_val_t Alignment>
struct aligned_allocator {
  using value_type = Ty;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using propagate_on_container_copy_assignment = true_type;
  using propagate_on_container_move_assignment = true_type;
  using propagate_on_container_swap = true_type;
  using is_always_equal = true_type;
  using enable_delete_null = true_type;

  Ty* allocate(size_t object_count) {
    return allocate_bytes(object_count * sizeof(value_type));
  }

  Ty* allocate_bytes(size_t bytes_count) {
    return static_cast<Ty*>(operator new (bytes_count, Alignment, NewTag{}));
  }

  void deallocate(Ty* ptr, size_t object_count) noexcept {
    deallocate_bytes(ptr, object_count * sizeof(value_type));
  }

  void deallocate_bytes(Ty* ptr, size_t bytes_count) noexcept {
    operator delete(ptr, bytes_count, Alignment);
  }
};

template <class Ty, class NewTag, align_val_t Alignment>
constexpr bool operator==(
    const aligned_allocator<Ty, NewTag, Alignment>&,
    const aligned_allocator<Ty, NewTag, Alignment>&) noexcept {
  return true;
}

template <class Ty, class NewTag, align_val_t Alignment>
constexpr bool operator!=(
    const aligned_allocator<Ty, NewTag, Alignment>&,
    const aligned_allocator<Ty, NewTag, Alignment>&) noexcept {
  return false;
}

template <class Ty>
using basic_paged_allocator =
    aligned_allocator<Ty,
                      paged_new_tag_t,
                      static_cast< ::std::align_val_t>(alignof(Ty))>;

template <class Ty>
using basic_non_paged_allocator =
    aligned_allocator<Ty,
                      non_paged_new_tag_t,
                      static_cast<::std::align_val_t>(alignof(Ty))>;

template <class Ty, crt::pool_type_t PoolType, ::std::align_val_t Alignment>
class tagged_allocator {
  using pool_tag_t = crt::pool_tag_t;

 public:
  using value_type = Ty;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using propagate_on_container_copy_assignment = true_type;
  using propagate_on_container_move_assignment = true_type;
  using propagate_on_container_swap = true_type;
  using is_always_equal = false_type;
  using enable_delete_null = true_type;

  constexpr explicit tagged_allocator(pool_tag_t pool_tag) noexcept
      : m_pool_tag{pool_tag} {}

  Ty* allocate(size_t object_count) {
    return allocate_bytes(object_count * sizeof(value_type));
  }

  Ty* allocate_bytes(size_t bytes_count) {
    return static_cast<Ty*>(allocate_bytes_impl(bytes_count));
  }

  void deallocate(Ty* ptr, size_t object_count) noexcept {
    deallocate_bytes(ptr, object_count * sizeof(value_type));
  }

  void deallocate_bytes(Ty* ptr, size_t bytes_count) noexcept {
    deallocate_memory(free_request_builder{ptr, bytes_count}
                          .set_alignment(Alignment)
                          .set_pool_tag(m_pool_tag)
                          .build());
  }

  void swap(tagged_allocator& other) noexcept {
    ktl::swap(m_pool_tag, other.m_pool_tag);
  }

  [[nodiscard]] constexpr pool_tag_t get_pool_tag() const noexcept {
    return m_pool_tag;
  }

private:
  void* allocate_bytes_impl(size_t bytes_count) {
    void* const buffer{allocate_memory(alloc_request_builder{bytes_count, PoolType}
                            .set_alignment(Alignment)
                            .set_pool_tag(m_pool_tag)
                            .build())};
    if (!buffer) {
      throw bad_alloc{};
    }
    return buffer;
  }

 private:
  pool_tag_t m_pool_tag;
};

template <class Ty, crt::pool_type_t PoolType, ::std::align_val_t Alignment>
constexpr bool operator==(
    const tagged_allocator<Ty, PoolType, Alignment>& lhs,
    const tagged_allocator<Ty, PoolType, Alignment>& rhs) noexcept {
  return lhs.get_pool_tag() == rhs.get_pool_tag();
}

template <class Ty, crt::pool_type_t PoolType, ::std::align_val_t Alignment>
constexpr bool operator!=(
    const tagged_allocator<Ty, PoolType, Alignment>& lhs,
    const tagged_allocator<Ty, PoolType, Alignment>& rhs) noexcept {
  return !(lhs == rhs);
}

template <class Ty, crt::pool_type_t PoolType, ::std::align_val_t Alignment>
void swap(tagged_allocator<Ty, PoolType, Alignment>& lhs,
          tagged_allocator<Ty, PoolType, Alignment>& rhs) noexcept {
  lhs.swap(rhs);
}

template <class Ty>
using tagged_paged_allocator =
    tagged_allocator<Ty,
                     PagedPool,
                     static_cast< ::std::align_val_t>(alignof(Ty))>;

template <class Ty>
using tagged_non_paged_allocator =
    tagged_allocator<Ty,
                     NonPagedPool,
                     static_cast< ::std::align_val_t>(alignof(Ty))>;

template <class Alloc>
struct allocator_traits {
  using allocator_type = Alloc;
  using value_type = typename Alloc::value_type;
  using pointer = mm::details::get_pointer_type_t<Alloc, value_type>;
  using const_pointer =
      mm::details::get_const_pointer_type_t<Alloc, value_type>;
  using reference = mm::details::get_reference_type_t<Alloc, value_type>;
  using const_reference =
      mm::details::get_const_reference_type_t<Alloc, value_type>;
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
    using other =
        allocator_traits<typename get_rebind_alloc_type<OtherTy>::type>;
  };

 public:
  template <class OtherTy>
  using rebind_alloc = typename get_rebind_alloc_type<OtherTy>::type;

  template <class OtherTy>
  using rebind_traits = typename get_rebind_traits_type<OtherTy>::type;

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
  static constexpr pointer construct(
      [[maybe_unused]] allocator_type& alloc,
      pointer ptr,
      Types&&... args) noexcept(is_nothrow_constructible_v<value_type,
                                                           Types...>) {
    if constexpr (mm::details::has_construct_v<allocator_type, pointer,
                                               Types...>) {
      return alloc_construct(alloc, ptr, forward<Types>(args)...);
    } else {
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

  static constexpr size_type max_size(const allocator_type& alloc) noexcept {
    if constexpr (mm::details::has_max_size_v<allocator_type>) {
      return alloc.max_size();
    } else {
      return (numeric_limits<size_type>::max)() / sizeof(value_type);
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

namespace alc::details {
template <class Allocator>
constexpr bool allocators_are_equal(
    const Allocator& lhs,
    const Allocator& rhs) noexcept(noexcept(lhs == rhs)) {
  if constexpr (allocator_traits<Allocator>::is_always_equal::value) {
    return true;
  } else {
    return lhs == rhs;
  }
}
}  // namespace alc::details
}  // namespace ktl
#endif
