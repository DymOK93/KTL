#pragma once
#include <basic_types.h>
#include <ntddk.h>

// TODO: вынести в .cpp
#define KTL_RUNTIME_DBG

//Защита на случай подключения стандартного хедера <memory>
#if defined(KTL_NO_CXX_STANDARD_LIBRARY) && !defined(__PLACEMENT_NEW_INLINE)
void* _cdecl operator new(size_t bytes_count, void* ptr) noexcept {
  (void)bytes_count;
  return ptr;
}
#endif

namespace ktl {
namespace mm::details {
// MSDN: Each ASCII character in the tag must be a value in the range 0x20
// (space) to 0x7E (tilde)
using memory_tag_t = uint16_t;
inline constexpr memory_tag_t tag_base{0x20}, tag_paged{tag_base},
    tag_non_paged{tag_base + 1};

template <memory_tag_t tag>
constexpr POOL_TYPE PoolTypeFromTag() {
  static_assert(tag >= tag_base, "Invalid tag");
  constexpr POOL_TYPE pool_type_info[] = {PagedPool, NonPagedPool};
  return pool_type_info[tag - tag_base];
}

#if defined(_WIN64)
inline constexpr size_t allocation_alignment{16};
#elif defined(_WIN32)
inline constexpr size_t allocation_alignment{8};
#else
#error Unsupported platform
#endif

struct __declspec(align(allocation_alignment)) MemoryBlockHeader {
  memory_tag_t tag{0};
};

inline void* ZeroFill(void* ptr, size_t size) {
  return RtlZeroMemory(ptr, size);  //Очистка неинициализированной памяти во
                                    //избежание утечки системных данных
}

template <uint32_t tag, class KernelAllocFunction>
void* allocate_impl(KernelAllocFunction alloc_func,
                                 size_t bytes_count) {
  if (!bytes_count) {
    return nullptr;
  }
  constexpr size_t header_size{sizeof(MemoryBlockHeader)};
  auto* memory_block_header{
      alloc_func(PoolTypeFromTag<tag>(), bytes_count + header_size, tag)};
  void* memory_block{nullptr};

  if (memory_block_header) {
    new (ZeroFill(memory_block_header, bytes_count)) MemoryBlockHeader{tag};
    memory_block = static_cast<byte*>(memory_block_header) + header_size;
#ifdef KTL_RUNTIME_DBG
    NT_ASSERT(reinterpret_cast<size_t>(memory_block) % allocation_alignment ==
              0);  // Пользовательские данные должны быть выровнены
#endif
  }
  return memory_block;
}

void deallocate_impl(void* memory_block) {
#ifdef KTL_RUNTIME_DBG
  NT_ASSERT(reinterpret_cast<size_t>(memory_block) % allocation_alignment == 0);
#endif
  auto* raw_header{static_cast<byte*>(memory_block) - allocation_alignment};
  auto* memory_block_header{reinterpret_cast<MemoryBlockHeader*>(raw_header)};
#ifdef KTL_RUNTIME_DBG
  NT_ASSERT(memory_block_header->tag >= mm::details::tag_base);
// Пользовательские данные должны быть выровнены
#endif
  ExFreePoolWithTag(memory_block_header, memory_block_header->tag);
}

}  // namespace mm::details

inline void* alloc_paged(size_t bytes_count) {
  return mm::details::allocate_impl<mm::details::tag_paged>(
      ExAllocatePoolWithTag, bytes_count);
}

inline void* alloc_non_paged(size_t bytes_count) {
  return mm::details::allocate_impl<mm::details::tag_non_paged>(
      ExAllocatePoolWithTag, bytes_count);
}

inline void deallocate(void* memory_block) {
  if (memory_block) {
    mm::details::deallocate_impl(memory_block);
  }
}

inline void free(void* ptr, size_t bytes = 0) {
  (void)bytes;  // unused
  deallocate(ptr);
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

}  // namespace ktl

void* _cdecl operator new(size_t bytes,
                          const nothrow_t&,
                          ktl::paged_new_tag_t) noexcept {
  return ktl::alloc_paged(bytes);
}

void* _cdecl operator new(size_t bytes,
                          const nothrow_t&,
                          ktl::non_paged_new_tag_t) noexcept {
  return ktl::alloc_non_paged(bytes);
}

void* _cdecl operator new(size_t bytes, const nothrow_t&) noexcept {
#ifdef KTL_USING_NON_PAGED_NEW_AS_DEFAULT
  return ktl::alloc_non_paged(
      bytes);  //По умолчанию память выделяется в paged-пуле
#else
  return ktl::alloc_paged(bytes);
#endif
}

void _cdecl operator delete(void* ptr, size_t bytes_count) {
  ktl::free(ptr, bytes_count);
}

void _cdecl operator delete(void* ptr) {
  ktl::free(ptr);
}