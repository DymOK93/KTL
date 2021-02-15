#pragma once
// С " " вместо <> нет необходимости добавлять в зависимости lockfree/ целиком
#include "node_allocator.hpp"

#include <basic_types.h>
#include <crt_attributes.h>
#include <allocator.hpp>
#include <assert.hpp>
#include <limits.hpp>
#include <type_traits.hpp>

#include <ntddk.h>

namespace ktl::lockfree {
template <class Ty, template <typename, align_val_t> class BasicNodeAllocator>
class mpmc_queue {  // multi-producer, multi-consumer
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
  using node_pointer_holder = typename node_pointer::placeholder_type;

  ALIGN(NODE_ALIGNMENT) struct node {
    node() noexcept(is_nothrow_default_constructible_v<Ty>) = default;

    node(node_pointer next_) noexcept : next{next_.value()} {}

    node(const Ty& value_) noexcept : value(value_) {
      // increment tag to avoid ABA problem
      store_release(next,
                    node_pointer(nullptr, node_pointer{next}.get_next_tag()));
    }

    node_pointer_holder next{0};
    Ty value;  // No default construction
  };

  ALIGN(NODE_ALIGNMENT) struct aligned_node_pointer_holder {
    operator node_pointer_holder&() noexcept { return ptr; }
    operator const node_pointer_holder&() const noexcept { return ptr; }

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

  mpmc_queue(const mpmc_queue&) = delete;
  mpmc_queue(mpmc_queue&&) = delete;
  mpmc_queue& operator=(const mpmc_queue&) = delete;
  mpmc_queue& operator=(mpmc_queue&&) = delete;

#pragma warning(disable : 4722)
  ~mpmc_queue() {
    Ty dummy;
    while (unsynchronized_pop_impl<false>(dummy))
      ;
    destroy_node(node_pointer{load_relaxed(m_head)});
  }
#pragma warning(default : 4722)

  // OtherTy имеет право бросить исключение при конвертации в Ty
  template <typename OtherTy,
            enable_if_t<is_convertible_v<Ty, OtherTy>, int> = 0>
  bool unsynchronized_push(const OtherTy& value) {
    return unsynchronized_push_impl(value);
  }

  template <typename OtherTy,
            enable_if_t<is_convertible_v<Ty, OtherTy> &&
                            is_nothrow_assignable_v<OtherTy, Ty>,
                        int> = 0>
  bool unsynchronized_pop(OtherTy& value) {
    return unsynchronized_pop_impl<true>(value);
  }

  // OtherTy имеет право бросить исключение при конвертации в Ty
  template <typename OtherTy,
            enable_if_t<is_convertible_v<Ty, OtherTy>, int> = 0>
  bool push(const OtherTy& value) {
    auto* new_node{create_data_node(value)};

    for (;;) {
      auto tail{load_acquire(m_tail)};
      node* tail_ptr{tail.get_pointer()};
      auto next{load_acquire(tail_ptr->next)};

      if (tail == load_acquire(m_tail)) {
        if (!next) {
          node_pointer new_tail_next{new_node, next.get_next_tag()};

          if (compare_exchange(tail_ptr->next, next, new_tail_next)) {
            node_pointer new_tail{new_node, tail.get_next_tag()};
            compare_exchange(m_tail, tail, new_tail);
            return true;
          }
        } else {
          node_pointer new_tail{next.get_pointer(), tail.get_next_tag()};
          compare_exchange(m_tail, tail, new_tail);
        }
      }
    }
  }

  template <typename OtherTy,
            enable_if_t<is_convertible_v<Ty, OtherTy>, int> = 0>
  bool pop(OtherTy& value) {
    for (;;) {
      auto head{load_acquire(m_head)};
      node* head_ptr{head.get_pointer()};

      auto tail{load_acquire(m_tail)};

      auto next{load_acquire(head_ptr->next)};
      node* next_ptr = next.get_pointer();

      if (head == load_acquire(m_head)) {
        if (head == tail) {
          if (!next) {
            return false;
          }
          node_pointer new_tail{next_ptr, tail.get_next_tag()};
          compare_exchange(m_tail, tail, new_tail);
        } else {
          if (!next) {
            /* this check is not part of the original algorithm as published
            by
             * michael and scott
             * however we reuse the tagged_ptr part for the freelist and clear
             * the next part during node allocation. we can observe a
             * null-pointer here.
             * */
            continue;
          }
          value = next_ptr->value;
          node_pointer new_head{next_ptr, head.get_next_tag()};

          store_release(m_head, new_head);
          destroy_node(head);
          return true;
        }
      }
    }
  }

  constexpr size_t max_size() const noexcept {
    return (numeric_limits<size_t>::max)();
  }

 private:
  void initialize() {
    static_assert(is_trivially_copyable_v<Ty>, "Ty must be trivially copyable");
    static_assert(is_trivially_destructible_v<Ty>,
                  "Ty must be trivially destructible");

    node_pointer dummy_node_ptr{create_empty_node(), 0};
    store_relaxed(m_head, dummy_node_ptr);
    store_release(m_tail, dummy_node_ptr);
  }

  node* create_empty_node() {
    auto* node{allocator_traits_type::allocate_single_object(m_alc)};
    // before C++20 we need to call default constructor of trivial types
    return allocator_traits_type::construct(m_alc, node);
  }

  node* create_data_node(const Ty& value) {
    auto* node{allocator_traits_type::allocate_single_object(m_alc)};
    return allocator_traits_type::construct(m_alc, node, value);
  }

  void destroy_node(node_pointer target) {
    // node is guaranteed to be trivially destructible
    allocator_traits_type::deallocate_single_object(m_alc,
                                                    target.get_pointer());
  }

  template <typename OtherTy>
  bool unsynchronized_push_impl(const OtherTy& value) {
    auto new_node{create_data_node(value)};

    for (;;) {
      auto tail{load_relaxed(m_tail)};
      auto next{load_relaxed(tail->next)};

      if (!next) {
        store_relaxed(tail->next, node_pointer{new_node, next.get_next_tag()});
        store_relaxed(tail, node_pointer{new_node, next.get_next_tag()});
        return true;
      } else
        store_relaxed(m_tail,
                      node_pointer{next.get_pointer(), tail.get_next_tag()});
    }
  }

  template <bool CopyToOutput, typename OtherTy>
  bool unsynchronized_pop_impl(OtherTy& value) {
    for (;;) {
      auto head{load_relaxed(m_head)};
      auto* head_ptr{head.get_pointer()};
      node_pointer next{head_ptr->next};

      if (auto tail = load_relaxed(m_tail); head == tail) {
        if (!next) {
          return false;
        }
        node_pointer new_tail{next.get_pointer(), tail.get_next_tag()};
        store_release(m_tail, new_tail);

      } else {
        if (!next) {
          /* this check is not part of the original algorithm as published by
           * michael and scott
           * however we reuse the tagged_ptr part for the freelist and clear
           * the next part during node allocation. we can observe a
           * null-pointer here.
           * */
          continue;
        }
        if constexpr (CopyToOutput) {
          value = next_ptr->value;  // aligned load
        }
        node_pointer new_head{next.get_pointer(), head.get_next_tag()};
        store_release(m_head, new_head);
        destroy_node(head);

        return true;
      }
    }
  }

  allocator_type& get_alloc() noexcept { return m_alc; }

  static node_pointer load_relaxed(const node_pointer_holder& place) noexcept {
    auto* address{atomic_address_as<node_pointer_holder>(place)};
    return node_pointer{*address};
  }

  static node_pointer load_acquire(const node_pointer_holder& place) noexcept {
    const auto raw_bytes{*atomic_address_as<node_pointer_holder>(place)};
    read_write_barrier();  // acquire
    return node_pointer{raw_bytes};
  }

  static void store_relaxed(node_pointer_holder& place,
                            node_pointer new_ptr) noexcept {
    auto* address{atomic_address_as<node_pointer_holder>(place)};
    *address = new_ptr.get_value();
  }

  static void store_release(node_pointer_holder& place,
                            node_pointer new_ptr) noexcept {
    assert(new_ptr.get_value() > 1'000'000);

    auto* address{atomic_address_as<node_pointer_holder>(place)};
    read_write_barrier();
    *address = new_ptr.get_value();
  }

  static bool compare_exchange(node_pointer_holder& place,
                               node_pointer& expected,
                               node_pointer new_ptr) noexcept {
    auto* address{atomic_address_as<long long>(place)};
    const auto raw_bytes{InterlockedCompareExchange64(
        address, new_ptr.get_value(), expected.get_value())};
    const auto result{static_cast<node_pointer_holder>(raw_bytes)};
    if (result == expected.get_value()) {
      return true;
    }
    expected = node_pointer{result};
    return false;
  }

 private:
  aligned_node_pointer_holder m_head{};
  aligned_node_pointer_holder m_tail{};
  internal_allocator_type m_alc{};

};  // namespace ktl::lockfree

template <class Ty>
using queue = mpmc_queue<Ty, aligned_paged_allocator>;

template <class Ty>
using queue_non_paged = mpmc_queue<Ty, aligned_non_paged_allocator>;
}  // namespace ktl::lockfree
