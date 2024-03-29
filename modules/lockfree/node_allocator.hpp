﻿#pragma once
// С " " вместо <> нет необходимости добавлять в зависимости lockfree/ целиком
#include "tagged_pointer.hpp"

#include <basic_types.hpp>
#include <crt_attributes.hpp>
#include <algorithm.hpp>
#include <allocator.hpp>
#include <atomic.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

#include <ntddk.h>

namespace ktl::lockfree {
template <class Ty,
          align_val_t Align,
          template <typename, align_val_t>
          class BasicNodeAllocator>
class node_allocator {
 public:
  using value_type = Ty;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

 private:
  static constexpr auto NODE_ALIGNMENT{static_cast<align_val_t>(
      (max)(crt::CACHE_LINE_SIZE, static_cast<size_type>(Align)))};

 private:
  struct memory_block_header;
  using node_pointer = tagged_pointer<memory_block_header>;
  using node_pointer_holder = atomic<typename node_pointer::placeholder_type>;

  struct memory_block_header {
    node_pointer_holder next{};
  };

 private:
  /*
   * Чтобы не увеличивать размер узла, зададим выравнивание непосредственно
   * в аллокатора
   * sizeof(aligned_storage_t<X, Y>) == Y при Y > X && Y >
   * alignof(max_align_val_t)
   */
  using memory_block =
      aligned_storage_t<(max)(sizeof(memory_block_header), sizeof(Ty)), 1>;

 public:
  using allocator_type = BasicNodeAllocator<memory_block, NODE_ALIGNMENT>;
  using allocator_traits_type = allocator_traits<allocator_type>;

  using propagate_on_container_copy_assignment = false_type;
  using propagate_on_container_move_assignment = false_type;
  using propagate_on_container_swap = false_type;
  using is_always_equal = false_type;

 public:
  node_allocator() noexcept = default;

  node_allocator(const allocator_type& alloc) noexcept(
      is_nothrow_copy_constructible_v<allocator_type>)
      : m_freelist{one_then_variadic_args{}, alloc} {}

  node_allocator(allocator_type&& alloc) noexcept(
      is_nothrow_move_constructible_v<allocator_type>)
      : m_freelist{one_then_variadic_args{}, move(alloc)} {}

  template <class Allocator = allocator_type>
  node_allocator(size_type initial_count, Allocator&& alloc = Allocator{})
      : m_freelist{one_then_variadic_args{}, forward<Allocator>(alloc)} {
    for (size_type idx = 0; idx < initial_count; ++idx) {
      push_unsafe(create_memory_block());
    }
  }

  node_allocator(const node_allocator&) = delete;
  node_allocator(node_allocator&&) = delete;
  node_allocator& operator=(const node_allocator&) = delete;
  node_allocator& operator=(node_allocator&&) = delete;

  ~node_allocator() {
    for (;;) {
      if (auto target = pop_unsafe_without_allocation(); target) {
        destroy_memory_block(target);
      } else {
        break;
      }
    }
  }

  Ty* allocate() {
    auto& head{get_head()};
    auto old_top_value{head.load<memory_order_consume>()};

    Ty* ptr{nullptr};
    while (!ptr) {
      auto old_top{node_pointer{old_top_value}};
      if (!old_top) {
        ptr = create_memory_block();
      } else {
        memory_block_header* new_top_ptr =
            node_pointer{old_top->next}.get_pointer();
        node_pointer new_pool{new_top_ptr, old_top.get_next_tag()};

        // old_top_value may be rewritten
        if (head.compare_exchange_weak(old_top_value, new_pool.get_value())) {
          ptr = reinterpret_cast<Ty*>(old_top.get_pointer());
        }
      }
    }
    return ptr;
  }

  void deallocate(Ty* ptr) noexcept {
    auto& head{get_head()};
    auto old_top_value{head.load<memory_order_consume>()};

    auto* new_top_ptr = reinterpret_cast<memory_block_header*>(ptr);

    for (;;) {
      node_pointer new_top{new_top_ptr, node_pointer{old_top_value}.get_tag()};
      new_top->next = old_top_value;

      // old_top_value may be rewritten
      if (head.compare_exchange_weak(old_top_value, new_top.get_value())) {
        break;
      }
    }
  }

 private:
  Ty* create_memory_block() {
    return reinterpret_cast<Ty*>(
        allocator_traits_type::allocate(get_alloc(), 1));
  }

  void destroy_memory_block(Ty* target) noexcept {
    allocator_traits_type::deallocate(
        get_alloc(), reinterpret_cast<memory_block*>(target), 1);
  }

  Ty* pop_unsafe_without_allocation() {
    auto& head{get_head()};
    auto old_top{node_pointer{head.load<memory_order_relaxed>()}};

    Ty* ptr{nullptr};
    if (old_top) {
      memory_block_header* new_top_ptr =
          node_pointer{old_top->next}.get_pointer();
      node_pointer new_top{new_top_ptr, old_top.get_next_tag()};
      head.store<memory_order_relaxed>(new_top.get_value());
      ptr = reinterpret_cast<Ty*>(old_top.get_pointer());
    }
    return ptr;
  }

  void push_unsafe(Ty* ptr) {
    auto& head{get_head()};
    auto* new_top_ptr = reinterpret_cast<memory_block_header*>(ptr);
    node_pointer current_head{head.load<memory_order_relaxed>()};
    node_pointer new_top{new_top_ptr, current_head.get_tag()};
    new_top->next.store<memory_order_relaxed>(
        node_pointer{current_head.get_pointer()}.get_value());
    head.store<memory_order_relaxed>(new_top.get_value());
  }

  allocator_type& get_alloc() noexcept { return m_freelist.get_first(); }

  node_pointer_holder& get_head() noexcept {
    return m_freelist.get_second().next;
  }

 private:
  compressed_pair<allocator_type, memory_block_header>
      m_freelist{};  // aligned tagged pointer
};
}  // namespace ktl::lockfree