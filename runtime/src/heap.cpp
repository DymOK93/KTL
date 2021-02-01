#include <heap.h>

namespace ktl {
namespace crt {
static constexpr size_t POOL_STORAGE_SIZE{DEFAULT_POOLS_IN_LIST_COUNT *
                                          sizeof(concurrent_node_allocator)},
    POOL_STORAGE_ALIGNMENT{alignof(concurrent_node_allocator)};

static aligned_storage_t<POOL_STORAGE_SIZE, POOL_STORAGE_ALIGNMENT>
    internal_pools[DEFAULT_POOL_LIST_COUNT];

memory_pool_list get_memory_pool_list_by_index(
    memory_pool_index_t idx) noexcept {
  auto* entry{internal_pools + idx};
  return {reinterpret_cast<concurrent_node_allocator*>(entry), idx};
}

memory_pool_index_t get_memory_pool_index(size_t bytes_count) noexcept {
  constexpr auto bit_scan_reverse{[](size_t value) -> uint32_t {
    unsigned long index;
    BITSCANREVERSE(&index, value);
    return index;
  }};
  const auto mask{bit_scan_reverse(bytes_count)};
  auto index{static_cast<memory_pool_index_t>(
      mask - MIN_POOL_NODE_SIZE_IN_PWR_OF_TWO)};
  if (bytes_count & ~(1ull << mask)) {
    ++index;
  }
  return index;
}

void deallocate_impl(void* memory_block, align_val_t alignment) noexcept {
  const size_t header_size{static_cast<size_t>(alignment)};
#ifdef KTL_RUNTIME_DBG
  assert(reinterpret_cast<size_t>(memory_block) % header_size == 0);
#endif
  auto* memory_block_header{reinterpret_cast<MemoryBlockHeader*>(
      static_cast<byte*>(memory_block) - header_size)};
  if (memory_block_header->not_from_pool) {
    ExFreePoolWithTag(memory_block_header, KTL_MEMORY_TAG);
  } else {
    auto& pool_list{
        get_memory_pool_list_by_index(memory_block_header->list_index).pools};
    pool_list[memory_block_header->pool_index].deallocate(memory_block_header);
  }
}

void construct_heap() {
  constexpr auto pool_initializer{[](memory_pool_type type,
                                     memory_pool_index_t idx,
                                     concurrent_node_allocator& pool) {
    new (&pool) concurrent_node_allocator(
        type, 1ull << (idx + MIN_POOL_NODE_SIZE_IN_PWR_OF_TWO));
  }};
  pool_storage_walker(pool_initializer);
}

void destroy_heap() noexcept {
  constexpr auto pool_destroyer{[]([[maybe_unused]] memory_pool_type type,
                                   [[maybe_unused]] memory_pool_index_t idx,
                                   concurrent_node_allocator& pool) {
    pool.~concurrent_node_allocator();
  }};
  pool_storage_walker(pool_destroyer);
}

}  // namespace crt

void* alloc_paged(size_t bytes_count, align_val_t alignment) noexcept {
  return crt::allocate_impl<crt::memory_pool_type::Paged>(bytes_count,
                                                          alignment);
}

void* alloc_non_paged(size_t bytes_count, align_val_t alignment) noexcept {
  return crt::allocate_impl<crt::memory_pool_type::NonPagedNx>(bytes_count,
                                                               alignment);
}

void deallocate(void* memory_block, align_val_t alignment) noexcept {
  if (memory_block) {
    crt::deallocate_impl(memory_block, alignment);
  }
}

void free(void* ptr, size_t, align_val_t alignment) noexcept {
  deallocate(ptr, alignment);
}
}  // namespace ktl