#pragma once
#include <basic_types.h>
#include <ntddk.h>

void* _cdecl operator new(size_t bytes_count, void* ptr) {
  (void)bytes_count;
  return ptr;
}

namespace winapi {
namespace kernel {
namespace mm {

namespace details {
inline constexpr uint32_t tag_paged{0}, tag_non_paged{1};
inline constexpr POOL_TYPE POOL_TYPE_FROM_TAG[2]{PagedPool, NonPagedPool};

struct MemoryBlockHeader {
  uint32_t tag{0};
};

inline void* zero_fill(void* ptr, size_t size) {
  return RtlZeroMemory(ptr, size);  //Очистка неинициализированной памяти во
                                    //избежание утечки системных данных
}

template <class KernelAllocFunction>
void* allocate(KernelAllocFunction alloc_func,
               uint32_t tag,
               size_t bytes_count) {
  if (!bytes_count) {
    return nullptr;
  }
  auto* memory_block{alloc_func(POOL_TYPE_FROM_TAG[tag], bytes_count, tag)};
  return new (zero_fill(memory_block, bytes_count)) MemoryBlockHeader{tag};
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

inline void free(void* ptr, size_t bytes = 0) {
  (void)bytes;  // unused
  details::deallocate(reinterpret_cast<details::MemoryBlockHeader*>(ptr));
}

inline wchar_t* allocate_string(size_t length_in_bytes) {
  return static_cast<wchar_t*>(alloc_paged(length_in_bytes));
}

inline void deallocate_string(PWCH str, size_t capacity) {
  free(str, capacity);
}

}  // namespace mm
}  // namespace kernel
}  // namespace winapi

void* _cdecl operator new(size_t bytes, const nothrow_t&) noexcept {
  return winapi::kernel::mm::alloc_paged(bytes);
}

void _cdecl operator delete(void* ptr, size_t bytes_count) {
  winapi::kernel::mm::free(ptr, bytes_count);
}

void _cdecl operator delete(void* ptr) {
  winapi::kernel::mm::free(ptr);
}