#pragma once
// С " " вместо <> нет необходимости добавлять в зависимости lockfree/ целиком
#include "node_allocator.hpp"

#include <basic_types.h>
#include <crt_attributes.h>
#include <allocator.hpp>
#include <assert.hpp>
#include <atomic.hpp>
#include <limits.hpp>
#include <type_traits.hpp>

#include <ntddk.h>

namespace ktl::lockfree {
template <class Ty, template <typename, align_val_t> class BasicNodeAllocator>
class mpmc_queue
    : public ktl::non_relocatable {  // multi-producer, multi-consumer
 public:
  using value_type = Ty;
  using reference = Ty&;
  using const_reference = const Ty&;
  using pointer = Ty*;
  using const_pointer = const Ty*;

 private:
  static constexpr auto NODE_ALIGNMENT{
      static_cast<align_val_t>((max)(crt::CACHE_LINE_SIZE, alignof(Ty)))};

 private:
  struct node;
  using node_pointer = tagged_pointer<node>;
  using node_pointer_holder = atomic<typename node_pointer::placeholder_type>;

  ALIGN(NODE_ALIGNMENT) struct node {
    node() noexcept(is_nothrow_default_constructible_v<Ty>) = default;

    node(node_pointer next_) noexcept : next{next_.value()} {}

    node(const Ty& value_) noexcept : value(value_) {
      // increment tag to avoid ABA problem
      next.store<memory_order_release>(
          node_pointer(nullptr, node_pointer{next}.get_next_tag()).get_value());
    }

    node_pointer_holder next{0};
    Ty value;  // No default construction
  };

  ALIGN(NODE_ALIGNMENT) struct aligned_node_pointer_holder {
    node_pointer_holder& get_ptr() noexcept { return ptr; }
    const node_pointer_holder& get_ptr() const noexcept { return ptr; }

    node_pointer_holder ptr{};
  };

  using internal_allocator_type = node_allocator<node, BasicNodeAllocator>;
  using allocator_traits_type = allocator_traits<internal_allocator_type>;

 public:
  using allocator_type =
      typename node_allocator<node, BasicNodeAllocator>::allocator_type;

  using size_type = typename internal_allocator_type::size_type;
  using difference_type = typename internal_allocator_type::difference_type;

 public:
  mpmc_queue() { initialize(); }
  mpmc_queue(const allocator_type& alloc) : m_alc(alloc) { initialize(); }
  mpmc_queue(allocator_type&& alloc) : m_alc(move(alloc)) { initialize(); }

  template <class Allocator = allocator_type>
  mpmc_queue(size_type initial_count, Allocator&& alloc)
      : m_alc(initial_count + 1, forward<Allocator>(alloc)) {
    initialize();
  }

  ~mpmc_queue() {
    Ty dummy;
    while (unsynchronized_pop_impl<false>(dummy))
      ;
    destroy_node(node_pointer{m_head.get_ptr().load<memory_order_relaxed>()});
  }

  // OtherTy имеет право бросить исключение при конвертации в Ty
  template <typename OtherTy,
            enable_if_t<is_constructible_v<Ty, OtherTy>, int> = 0>
  bool unsynchronized_push(const OtherTy& value) {
    return unsynchronized_push_impl(value);
  }

  template <typename OtherTy,
            enable_if_t<is_convertible_v<OtherTy, Ty> &&
                            is_nothrow_assignable_v<OtherTy&, Ty>,
                        int> = 0>
  bool unsynchronized_pop(OtherTy& value) {
    return unsynchronized_pop_impl<true>(value);
  }

  // OtherTy имеет право бросить исключение при конвертации в Ty
  template <typename OtherTy,
            enable_if_t<is_constructible_v<Ty, OtherTy>, int> = 0>
  bool push(const OtherTy& value) {
    auto* new_node{create_data_node(value)};

    for (;;) {
      auto tail{node_pointer{m_tail.get_ptr().load<memory_order_acquire>()}};
      node* tail_ptr{tail.get_pointer()};
      auto next{node_pointer{tail_ptr->next.load<memory_order_acquire>()}};

      node_pointer current_tail{m_tail.get_ptr().load<memory_order_acquire>()};
      if (tail == current_tail) {
        if (!next) {
          node_pointer new_tail_next{new_node, next.get_next_tag()};

          if (cas_weak_helper(tail_ptr->next, next, new_tail_next)) {
            node_pointer new_tail{new_node, tail.get_next_tag()};
            cas_strong_helper(m_tail.get_ptr(), tail, new_tail);
            return true;
          }
        } else {
          node_pointer new_tail{next.get_pointer(), tail.get_next_tag()};
          cas_strong_helper(m_tail.get_ptr(), tail, new_tail);
        }
      }
    }
  }

  template <typename OtherTy,
            enable_if_t<is_convertible_v<OtherTy, Ty> &&
                            is_nothrow_assignable_v<OtherTy&, Ty>,
                        int> = 0>
  bool pop(OtherTy& value) {
    for (;;) {
      auto head{node_pointer{m_head.get_ptr().load<memory_order_acquire>()}};
      node* head_ptr{head.get_pointer()};

      auto tail{node_pointer{m_tail.get_ptr().load<memory_order_acquire>()}};
      auto next{node_pointer{head_ptr->next.load<memory_order_acquire>()}};
      node* next_ptr = next.get_pointer();

      node_pointer current_head{m_head.get_ptr().load<memory_order_acquire>()};
      if (head == current_head) {
        if (head == tail) {
          if (!next) {
            return false;
          }
          node_pointer new_tail{next_ptr, tail.get_next_tag()};
          cas_strong_helper(m_tail.get_ptr(), tail, new_tail);
        } else {
          if (!next) {
            /*
             * This check is not part of the original algorithm as published
             * by michael and scott however we reuse the tagged_pointer part for
             * the freelist and clear he next part during node allocation
             * we can observe a null-pointer here
             */
            continue;
          }
          value = next_ptr->value;
          node_pointer new_head{next_ptr, head.get_next_tag()};

          if (cas_weak_helper(m_head.get_ptr(), head, new_head)) {
            destroy_node(head);
            return true;
          }
        }
      }
    }
  }

  constexpr size_t max_size() const noexcept {
    return (numeric_limits<size_t>::max)();
  }

 private:
  void initialize() {
    static_assert(is_trivially_destructible_v<Ty>,
                  "Ty must be trivially destructible");

    node_pointer dummy_node_ptr{create_empty_node(), 0};
    m_head.get_ptr().store<memory_order_relaxed>(dummy_node_ptr.get_value());
    m_tail.get_ptr().store<memory_order_release>(dummy_node_ptr.get_value());
  }

  node* create_empty_node() {
    auto* node{allocator_traits_type::allocate_single_object(m_alc)};
    // before C++20 we need to call default constructor of trivial types
    return allocator_traits_type::construct(m_alc, node);
  }

  node* create_data_node(const Ty& value) {
    auto* node{allocator_traits_type::allocate_single_object(m_alc)};
    ++m_allocated;
    return allocator_traits_type::construct(m_alc, node, value);
  }

  void destroy_node(node_pointer target) {
    // node is guaranteed to be trivially destructible
    allocator_traits_type::deallocate_single_object(m_alc,
                                                    target.get_pointer());
    ++m_freed;
  }

  template <typename OtherTy>
  bool unsynchronized_push_impl(const OtherTy& value) {
    auto new_node{create_data_node(value)};

    for (;;) {
      auto tail{node_pointer{m_tail.get_ptr().load<memory_order_relaxed>()}};
      auto next{node_pointer{tail->next.load<memory_order_relaxed>()}};

      if (!next) {
        tail->next.store<memory_order_relaxed>(
            node_pointer{new_node, next.get_next_tag()}.get_value());
        m_tail.get_ptr().store<memory_order_relaxed>(
            node_pointer{new_node, next.get_next_tag()}.get_value());
        return true;
      } else
        m_tail.get_ptr().store<memory_order_relaxed>(
            node_pointer{next.get_pointer(), tail.get_next_tag()}.get_value());
    }
  }

  template <bool CopyToOutput, typename OtherTy>
  bool unsynchronized_pop_impl(OtherTy& value) {
    for (;;) {
      auto head{node_pointer{m_head.get_ptr().load<memory_order_relaxed>()}};
      auto* head_ptr{head.get_pointer()};
      node_pointer next{head_ptr->next};

      if (auto tail =
              node_pointer{m_tail.get_ptr().load<memory_order_relaxed>()};
          head == tail) {
        if (!next) {
          return false;
        }
        node_pointer new_tail{next.get_pointer(), tail.get_next_tag()};
        m_tail.get_ptr().store<memory_order_release>(new_tail.get_value());

      } else {
        if (!next) {
          /*
           * This check is not part of the original algorithm as
           * published by michael and scott however we reuse the
           * tagged_pointer part for the freelist and clear he next
           * part during node allocation we can observe a
           * null-pointer here
           */
          continue;
        }
        auto next_ptr{next.get_pointer()};
        if constexpr (CopyToOutput) {
          value = next_ptr->value;  // aligned load
        }
        node_pointer new_head{next_ptr, head.get_next_tag()};
        m_head.get_ptr().store<memory_order_release>(new_head.get_value());
        destroy_node(head);

        return true;
      }
    }
  }

  allocator_type& get_alloc() noexcept { return m_alc; }

  static bool cas_strong_helper(node_pointer_holder& place,
                                node_pointer expected,
                                node_pointer new_ptr) noexcept {
    auto expected_value{expected.get_value()};
    return place.compare_exchange_strong(expected_value, new_ptr.get_value());
  }

  static bool cas_weak_helper(node_pointer_holder& place,
                              node_pointer expected,
                              node_pointer new_ptr) noexcept {
    auto expected_value{expected.get_value()};
    return place.compare_exchange_weak(expected_value, new_ptr.get_value());
  }

 private:
  aligned_node_pointer_holder m_head{};
  aligned_node_pointer_holder m_tail{};
  internal_allocator_type m_alc{};
  atomic_size_t m_allocated{0}, m_freed{0};

};  // namespace ktl::lockfree

template <class Ty>
using queue = mpmc_queue<Ty, aligned_paged_allocator>;

template <class Ty>
using queue_non_paged = mpmc_queue<Ty, aligned_non_paged_allocator>;
}  // namespace ktl::lockfree
