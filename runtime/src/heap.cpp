#include <heap.hpp>

namespace ktl {
namespace crt {
void deallocate_impl(void* memory_block,
                     [[maybe_unused]] std::align_val_t alignment) noexcept {
  ExFreePoolWithTag(memory_block, KTL_HEAP_TAG);
}
}  // namespace crt

void* alloc_paged(size_t bytes_count, std::align_val_t alignment) noexcept {
  if (alignment <= crt::DEFAULT_ALLOCATION_ALIGNMENT) {
    return crt::allocate_impl<PagedPool, crt::DEFAULT_ALLOCATION_ALIGNMENT>(
        bytes_count, alignment);
  } else {
    return crt::allocate_impl<PagedPoolCacheAligned,
                              crt::EXTENDED_ALLOCATION_ALIGNMENT>(bytes_count,
                                                                  alignment);
  }
}

void* alloc_non_paged(size_t bytes_count, std::align_val_t alignment) noexcept {
  if (alignment <= crt::DEFAULT_ALLOCATION_ALIGNMENT) {
    return crt::allocate_impl<NonPagedPoolNx,
                              crt::DEFAULT_ALLOCATION_ALIGNMENT>(bytes_count,
                                                                 alignment);
  } else {
    return crt::allocate_impl<NonPagedPoolCacheAligned,
                              crt::EXTENDED_ALLOCATION_ALIGNMENT>(bytes_count,
                                                                  alignment);
  }
}

void deallocate(void* memory_block, std::align_val_t alignment) noexcept {
  if (memory_block) {
    crt::deallocate_impl(memory_block, alignment);
  }
}

void free(void* ptr, std::align_val_t alignment) noexcept {
  deallocate(ptr, alignment);
}

void free(void* ptr, size_t, std::align_val_t alignment) noexcept {
  free(ptr, alignment);
}
}  // namespace ktl