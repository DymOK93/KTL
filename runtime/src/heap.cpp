#include <heap.h>

namespace ktl {
namespace crt {
void deallocate_impl(void* memory_block,
                     [[maybe_unused]] align_val_t alignment) noexcept {
  ExFreePoolWithTag(memory_block, KTL_HEAP_TAG);
}
}  // namespace crt

void* alloc_paged(size_t bytes_count, align_val_t alignment) noexcept {
  return crt::allocate_impl<PagedPool>(bytes_count, alignment);
}

void* alloc_non_paged(size_t bytes_count, align_val_t alignment) noexcept {
  return crt::allocate_impl<NonPagedPoolNx>(bytes_count, alignment);
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