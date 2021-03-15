#pragma once
// С " " вместо <> нет необходимости добавлять в зависимости lockfree/ целиком
#include "tagged_pointer.hpp"

#include <basic_types.h>
#include <crt_attributes.h>
#include <algorithm.hpp>
#include <allocator.hpp>
#include <atomic.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

#include <ntddk.h>

template <class>
struct Deductor;

namespace ktl::lockfree {
template <class Ty, template <typename, align_val_t> class BasicNodeAllocator>
class node_allocator {
 public:
  using value_type = Ty;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

 private:
  static constexpr auto NODE_ALIGNMENT{
      static_cast<align_val_t>((max)(crt::CACHE_LINE_SIZE, alignof(Ty)))};

 private:
  struct memory_block_header;
  using node_pointer = tagged_pointer<memory_block_header>;
  using node_pointer_holder = atomic<typename node_pointer::placeholder_type>;

  ALIGN(NODE_ALIGNMENT) struct memory_block_header {
    node_pointer_holder next{};
  };

  using memory_block =
      aligned_storage_t<(max)(sizeof(memory_block_header), sizeof(Ty)),
                        static_cast<size_t>(NODE_ALIGNMENT)>;

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
      ++m_allocated;
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
    auto old_top{node_pointer{head.load<memory_order_acquire>()}};

    Ty* ptr{nullptr};
    while (!ptr) {
      if (!old_top) {
        ptr = create_memory_block();
      } else {
        memory_block_header* new_top_ptr =
            node_pointer{old_top->next}.get_pointer();
        node_pointer new_pool{new_top_ptr, old_top.get_next_tag()};

        if (compare_exchange_helper(head, old_top, new_pool)) {
          ptr = reinterpret_cast<Ty*>(old_top.get_pointer());
        }
      }
    }
    return ptr;
  }

  void deallocate(Ty* ptr) noexcept {
    auto& head{get_head()};
    auto old_top{node_pointer{head.load<memory_order_acquire>()}};

    auto* new_top_ptr = reinterpret_cast<memory_block_header*>(ptr);

    for (;;) {
      node_pointer new_top{new_top_ptr, old_top.get_tag()};
      new_top->next = old_top.get_value();

      if (compare_exchange_helper(head, old_top, new_top)) {
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
    auto old_top{node_pointer{head.load<memory_order_acquire>()}};

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
    node_pointer new_top{new_top_ptr, head.get_tag()};
    new_top->next.set_pointer(old_pool.get_pointer());
    store_relaxed(head, new_top);
  }

  allocator_type& get_alloc() noexcept { return m_freelist.get_first(); }

  node_pointer_holder& get_head() noexcept {
    return m_freelist.get_second().next;
  }

  static bool compare_exchange_helper(node_pointer_holder& place,
                                      node_pointer expected,
                                      node_pointer new_ptr) {
    auto expected_value{expected.get_value()};
    return place.compare_exchange_strong(expected_value, new_ptr.get_value());
  }

 private:
  compressed_pair<allocator_type, memory_block_header>
      m_freelist{};  // aligned tagged pointer
};
}  // namespace ktl::lockfree