#include <heap.h>

namespace ktl {
namespace crt {
void* zero_fill(void* ptr, size_t size) noexcept {
  return RtlZeroMemory(ptr, size);  //«атирание во избежание утечки чужих данных
}

void deallocate_impl(void* memory_block) noexcept {
#ifdef KTL_RUNTIME_DBG
  NT_ASSERT(reinterpret_cast<size_t>(memory_block) % allocation_alignment == 0);
#endif
  auto* raw_header{static_cast<byte*>(memory_block) - allocation_alignment};
  auto* memory_block_header{reinterpret_cast<MemoryBlockHeader*>(raw_header)};
#ifdef KTL_RUNTIME_DBG
  NT_ASSERT(memory_block_header->tag >= tag_base);
#endif
  ExFreePoolWithTag(memory_block_header, memory_block_header->tag);
}
}  // namespace crt

void* alloc_paged(size_t bytes_count) noexcept {
  return crt::allocate_impl<crt::tag_paged>(ExAllocatePoolWithTag, bytes_count);
}

void* alloc_non_paged(size_t bytes_count) noexcept {
  return crt::allocate_impl<crt::tag_non_paged>(ExAllocatePoolWithTag,
                                                bytes_count);
}

void deallocate(void* memory_block) noexcept {
  if (memory_block) {
    crt::deallocate_impl(memory_block);
  }
}

void free(void* ptr, size_t) noexcept {
  deallocate(ptr);
}

wchar_t* allocate_string(size_t length_in_bytes) noexcept {
  return static_cast<wchar_t*>(alloc_paged(length_in_bytes));
}

void deallocate_string(wchar_t* str, size_t capacity) noexcept {
  free(str, capacity);
}
}  // namespace ktl