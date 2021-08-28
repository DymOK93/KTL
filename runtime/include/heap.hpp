#pragma once
#include <basic_types.h>
#include <algorithm_impl.hpp>

#include <ntddk.h>

namespace ktl {
namespace crt {
using memory_tag_t = uint32_t;
inline constexpr auto KTL_HEAP_TAG{'LTK'};  // Reversed 'KTL'

inline constexpr size_t MEMORY_PAGE_SIZE{PAGE_SIZE}, CACHE_LINE_SIZE{64};

inline constexpr auto DEFAULT_ALLOCATION_ALIGNMENT{
    static_cast<std::align_val_t>(2 * sizeof(size_t))},  // 8 on x86, 16 on x64
    EXTENDED_ALLOCATION_ALIGNMENT{
        static_cast<std::align_val_t>(CACHE_LINE_SIZE)},  // cache line
    MAX_ALLOCATION_ALIGNMENT{static_cast<std::align_val_t>(MEMORY_PAGE_SIZE)};

using native_memory_pool_t = POOL_TYPE;

template <native_memory_pool_t PoolType,
          std::align_val_t MaxAlignmentForThisPool>
void* allocate_impl(size_t bytes_count, std::align_val_t alignment) noexcept {
  if (alignment <= MaxAlignmentForThisPool) {
    return ExAllocatePoolUninitialized(PoolType, bytes_count, KTL_HEAP_TAG);
  }
  const size_t aligned_size{
      (max)(bytes_count, MEMORY_PAGE_SIZE)};  // Align to page size
  return ExAllocatePoolUninitialized(PoolType, aligned_size, KTL_HEAP_TAG);
}

void deallocate_impl(void* memory_block, std::align_val_t alignment) noexcept;
}  // namespace crt

void* alloc_paged(
    size_t bytes_count,
    std::align_val_t alignment = crt::DEFAULT_ALLOCATION_ALIGNMENT) noexcept;

void* alloc_non_paged(
    size_t bytes_count,
    std::align_val_t alignment = crt::DEFAULT_ALLOCATION_ALIGNMENT) noexcept;

void deallocate(
    void* memory_block,
    std::align_val_t alignment = crt::DEFAULT_ALLOCATION_ALIGNMENT) noexcept;

void free(void* ptr, std::align_val_t alignment) noexcept;

void free(
    void* ptr,
    size_t bytes = 0,
    std::align_val_t alignment = crt::DEFAULT_ALLOCATION_ALIGNMENT) noexcept;
}  // namespace ktl
