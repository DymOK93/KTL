#pragma once
#include <basic_types.h>
#include <crt_attributes.h>
#include <placement_new.h>

#include <ntddk.h>

namespace ktl {
namespace crt {
// MSDN: Each ASCII character in the tag must be a value in the range 0x20
// (space) to 0x7E (tilde)
using memory_tag_t = uint16_t;
inline constexpr memory_tag_t tag_base{0x20}, tag_paged{tag_base},
    tag_non_paged{tag_base + 1};

template <memory_tag_t tag>
constexpr POOL_TYPE PoolTypeFromTag() {
  static_assert(tag >= tag_base, "Invalid tag");
  constexpr POOL_TYPE pool_type_info[] = {PagedPool, NonPagedPoolNx};
  return pool_type_info[tag - tag_base];
}

#if defined(_WIN64)
inline constexpr size_t allocation_alignment{16};
#elif defined(_WIN32)
inline constexpr size_t allocation_alignment{8};
#else
#error Unsupported platform
#endif

struct alignas(allocation_alignment) MemoryBlockHeader {
  memory_tag_t tag{0};
};

inline void* zero_fill(void* ptr, size_t size);

template <memory_tag_t tag, class KernelAllocFunction>
void* allocate_impl(KernelAllocFunction alloc_func, size_t bytes_count) {
  if (!bytes_count) {
    return nullptr;
  }
  constexpr size_t header_size{sizeof(MemoryBlockHeader)};
  auto* memory_block_header{
      alloc_func(PoolTypeFromTag<tag>(), bytes_count + header_size, tag)};
  void* memory_block{nullptr};

  if (memory_block_header) {
    new (zero_fill(memory_block_header, bytes_count)) MemoryBlockHeader{tag};
    memory_block = static_cast<byte*>(memory_block_header) + header_size;
#ifdef KTL_RUNTIME_DBG
    NT_ASSERT(reinterpret_cast<size_t>(memory_block) % allocation_alignment ==
              0);
#endif
  }
  return memory_block;
}

void deallocate_impl(void* memory_block);

}  // namespace crt

void* alloc_paged(size_t bytes_count);
void* alloc_non_paged(size_t bytes_count);
void deallocate(void* memory_block);
void free(void* ptr, size_t bytes = 0);
wchar_t* allocate_wide_string(size_t length_in_bytes);
void deallocate_wide_string(wchar_t* str, size_t capacity);
}  // namespace ktl
