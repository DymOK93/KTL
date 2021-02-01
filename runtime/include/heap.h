#pragma once
#include <assert.h>
#include <basic_types.h>
#include <concurrent_node_allocator.h>
#include <crt_attributes.h>
#include <placement_new.h>
#include <heap_impl.hpp>
#include <intrinsic.hpp>
#include <limits_impl.hpp>
#include <type_traits_impl.hpp>

#include <ntddk.h>

namespace ktl {
namespace crt {
using memory_pool_index_t = uint8_t;

struct MemoryBlockHeader {
  memory_pool_index_t not_from_pool : 1;
  memory_pool_index_t list_index : 3;
  memory_pool_index_t pool_index : 4;
};

/**
 * Pool index layout:
 * [7 6 5 4 3 2 1 0]
 * bist 7 - not-a-pool tag
 * bits [6..4] - pool list index (0111 is bad index)
 * bits [3...0] - pool index in list
 */

inline constexpr size_t MAX_POOL_LISTS_COUNT{(1u << 3) - 1},
    MAX_POOLS_IN_LIST_COUNT{(1u << 4) - 1},
    DEFAULT_POOL_LIST_COUNT{static_cast<size_t>(memory_pool_type::_Count)},
    DEFAULT_POOLS_IN_LIST_COUNT{8}, MIN_POOL_NODE_SIZE_IN_PWR_OF_TWO{5},
    MIN_POOL_NODE_SIZE_IN_BYTES{1u << MIN_POOL_NODE_SIZE_IN_PWR_OF_TWO};

struct memory_pool_list {
  concurrent_node_allocator* pools;
  memory_pool_index_t index;
};

memory_pool_list get_memory_pool_list_by_index(
    memory_pool_index_t idx) noexcept;

template <memory_pool_type type>
memory_pool_list get_memory_pool_list_by_type() noexcept {
  return get_memory_pool_list_by_index(static_cast<size_t>(type));
}

memory_pool_index_t get_memory_pool_index(size_t bytes_count) noexcept;

template <memory_pool_type type>
void* allocate_impl(size_t bytes_count, align_val_t alignment) noexcept {
  if (!bytes_count) {
    return nullptr;
  }

  const size_t header_size{static_cast<size_t>(alignment)};
  const size_t summary_size{bytes_count + header_size};

  void* memory_block_header{nullptr};
  MemoryBlockHeader header{};

  if (summary_size >= MEMORY_PAGE_SIZE ||
      header_size > static_cast<size_t>(MAX_ALLOCATION_ALIGNMENT)) {
    memory_block_header = ExAllocatePoolWithTag(to_native_pool_type(type),
                                                summary_size, KTL_MEMORY_TAG);
    header.not_from_pool = 1;
  } else {
    memory_pool_index_t pool_idx{get_memory_pool_index(summary_size)};
    auto [pool_list_ptr, pool_list_idx]{get_memory_pool_list_by_type<type>()};
    memory_block_header = pool_list_ptr[pool_idx].allocate();
    header.list_index = pool_list_idx;
    header.pool_index = pool_idx;
  }

  void* memory_block{nullptr};

  if (memory_block_header) {
    new (memory_block_header) MemoryBlockHeader{header};
    memory_block = static_cast<byte*>(memory_block_header) + header_size;
  }

  return memory_block;
}

void deallocate_impl(void* memory_block, align_val_t alignment) noexcept;

template <class Handler>
void pool_storage_walker(Handler handler) {
  for (memory_pool_index_t pool_list_idx = 0;
       pool_list_idx < DEFAULT_POOL_LIST_COUNT; ++pool_list_idx) {
    auto pool_type{static_cast<memory_pool_type>(pool_list_idx)};
    auto [pool_list, _]{get_memory_pool_list_by_index(pool_list_idx)};
    for (memory_pool_index_t pool_idx = 0;
         pool_idx < DEFAULT_POOLS_IN_LIST_COUNT; ++pool_idx) {
      handler(pool_type, pool_idx, pool_list[pool_idx]);
    }
  }
}

void construct_heap();  // Должна быть вызвана перед любыми аллокациями памяти
void destroy_heap() noexcept;  // Должна быть вызвана по завершении работы

}  // namespace crt

void* alloc_paged(
    size_t bytes_count,
    align_val_t alignment = crt::DEFAULT_ALLOCATION_ALIGNMENT) noexcept;

void* alloc_non_paged(
    size_t bytes_count,
    align_val_t alignment = crt::DEFAULT_ALLOCATION_ALIGNMENT) noexcept;

void deallocate(
    void* memory_block,
    align_val_t alignment = crt::DEFAULT_ALLOCATION_ALIGNMENT) noexcept;

void free(void* ptr,
          size_t bytes = 0,
          align_val_t alignment = crt::DEFAULT_ALLOCATION_ALIGNMENT) noexcept;
}  // namespace ktl
