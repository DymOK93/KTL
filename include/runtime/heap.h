#pragma once
#include <basic_types.h>
#include <ntddk.h>

// TODO: вынести в .cpp

void* _cdecl operator new(size_t bytes_count, void* ptr) noexcept {
  (void)bytes_count;
  return ptr;
}

namespace winapi {
namespace kernel {
namespace mm {

namespace details {
// MSDN: Each ASCII character in the tag must be a value in the range 0x20
// (space) to 0x7E (tilde)
inline constexpr uint32_t tag_base{0x20}, tag_paged{tag_base},
    tag_non_paged{tag_base + 1};

template <uint32_t tag>
constexpr POOL_TYPE PoolTypeFromTag() {
  static_assert(tag >= tag_base, "Invalid tag");
  constexpr POOL_TYPE pool_type_info[2]{PagedPool, NonPagedPool};
  return pool_type_info[tag - tag_base];
}

struct MemoryBlockHeader {
  uint32_t tag{0};
};

inline void* ZeroFill(void* ptr, size_t size) {
  return RtlZeroMemory(ptr, size);  //Очистка неинициализированной памяти во
                                    //избежание утечки системных данных
}

template <uint32_t tag, class KernelAllocFunction>
MemoryBlockHeader* allocate(KernelAllocFunction alloc_func,
                            size_t bytes_count) {
  if (!bytes_count) {
    return nullptr;
  }
  auto* memory_block{alloc_func(PoolTypeFromTag<tag>(), bytes_count, tag)};
  return new (ZeroFill(memory_block, bytes_count)) MemoryBlockHeader{tag};
}
}  // namespace details

inline void* alloc_paged(size_t bytes_count) {
  return details::allocate<details::tag_paged>(ExAllocatePoolWithTag,
                                               bytes_count);
}

inline void* alloc_non_paged(size_t bytes_count) {
  return details::allocate<details::tag_non_paged>(ExAllocatePoolWithTag,
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

inline void deallocate_string(wchar_t* str, size_t capacity) {
  free(str, capacity);
}

struct paged_new_tag_t {};
inline constexpr paged_new_tag_t paged_new;

struct non_paged_new_tag_t {};
inline constexpr non_paged_new_tag_t non_paged_new;

}  // namespace mm
}  // namespace kernel
}  // namespace winapi

void* _cdecl operator new(size_t bytes,
                          const nothrow_t&,
                          winapi::kernel::mm::paged_new_tag_t) noexcept {
  return winapi::kernel::mm::alloc_paged(bytes);
}

void* _cdecl operator new(size_t bytes,
                          const nothrow_t&,
                          winapi::kernel::mm::non_paged_new_tag_t) noexcept {
  return winapi::kernel::mm::alloc_non_paged(bytes);
}

void* _cdecl operator new(size_t bytes, const nothrow_t&) noexcept {
#ifdef USING_NON_PAGED_NEW_AS_DEFAULT
  return winapi::kernel::mm::alloc_non_paged(
      bytes);  //По умолчанию память выделяется в paged-пуле
#else
  return winapi::kernel::mm::alloc_paged(bytes);
#endif
}

void _cdecl operator delete(void* ptr, size_t bytes_count) {
  winapi::kernel::mm::free(ptr, bytes_count);
}

void _cdecl operator delete(void* ptr) {
  winapi::kernel::mm::free(ptr);
}