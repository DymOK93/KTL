#pragma once
#include <basic_types.h>
#include <ntddk.h>

namespace ktl::crt {
// MSDN: Each ASCII character in the tag must be a value in the range 0x20
// (space) to 0x7E (tilde)
using memory_tag_t = uint32_t;
inline constexpr auto KTL_MEMORY_TAG{'LTK'};  // Reversed 'KTL'

inline constexpr size_t MEMORY_PAGE_SIZE{PAGE_SIZE};

inline constexpr auto DEFAULT_ALLOCATION_ALIGNMENT{
    static_cast<align_val_t>(alignof(max_align_t))},
    MAX_ALLOCATION_ALIGNMENT{static_cast<align_val_t>(32)},
    EXTENDED_ALLOCATION_ALIGNMENT{static_cast<align_val_t>(MEMORY_PAGE_SIZE)};

using native_memoty_pool_t = POOL_TYPE;

enum class memory_pool_type { Paged, NonPagedNx, NonPagedExecute, _Count };

constexpr native_memoty_pool_t to_native_pool_type(
    memory_pool_type type) noexcept {
  native_memoty_pool_t converted_type{};
#define GENERATE_POOL_TYPE_CONVERSION_CASE(type, native_type) \
  case memory_pool_type::##type:                              \
    converted_type = native_type;                             \
    break;
  switch (type) {
    GENERATE_POOL_TYPE_CONVERSION_CASE(Paged, PagedPool)
    GENERATE_POOL_TYPE_CONVERSION_CASE(NonPagedNx, NonPagedPoolNx)
    GENERATE_POOL_TYPE_CONVERSION_CASE(NonPagedExecute, NonPagedPool)
    default:
      assert(false && "Bad pool type");
      break;
  }
#undef GENERATE_POOL_TYPE_CONVERSION_CASE
  return converted_type;
}
}  // namespace ktl::crt