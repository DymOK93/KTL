#pragma once
#include <ntddk.h>
#include <array>
#include <cstdint>
#include <functional>
#include <new>

namespace winapi {
namespace kernel {
namespace mm {

namespace details {
static constexpr uint32_t tag_paged{0}, tag_non_paged{1};

std::array<POOL_TYPE, 2> POOL_TYPE_FROM_TAG{{PagedPool, NonPagedPool}};

struct MemoryBlockHeader {
  uint32_t tag{0};
};

template <class KernelAllocFunction>
void* allocate(KernelAllocFunction alloc_func,
               uint32_t tag,
               size_t bytes_count) {
  if (!bytes_count) {
    return nullptr;
  }
  auto* memory_block{
      std::invoke(alloc_func, POOL_TYPE_FROM_TAG[tag], bytes_count, tag)};
  return new (memory_block) MemoryBlockHeader{tag};
}
}  // namespace details

inline void* alloc_paged(size_t bytes_count) {
  return details::allocate(ExAllocatePoolWithTag, details::tag_paged,
                           bytes_count);
}

inline void* alloc_non_paged(size_t bytes_count) {
  return details::allocate(ExAllocatePoolWithTag, details::tag_non_paged,
                           bytes_count);
}

namespace details {
inline void deallocate(MemoryBlockHeader* memory_block) {
  if (memory_block) {
    ExFreePoolWithTag(memory_block, memory_block->tag);
  }
}
}  // namespace details

inline void free(void* ptr, size_t bytes) {
  (void)bytes;  // unused
  details::deallocate(reinterpret_cast<details::MemoryBlockHeader*>(ptr));
}

inline PWCH allocate_string(size_t length_in_bytes) {
  return static_cast<PWCH>(alloc_paged(length_in_bytes));
}

inline void deallocate_string(PWCH str, size_t capacity) {
  free(str, capacity);
}

}  // namespace mm
}  // namespace kernel
}  // namespace winapi

void* operator new(size_t bytes, std::nothrow_t) {
  return winapi::kernel::mm::alloc_paged(bytes);
}

void operator delete(void* ptr, size_t bytes) noexcept {
  winapi::kernel::mm::free(ptr, bytes);
}