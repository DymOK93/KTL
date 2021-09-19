#pragma once
#include <basic_types.hpp>
#include <crt_assert.hpp>

#include <ntddk.h>

#ifdef TARGET_WINVER
  #if TARGET_WINVER >= _WIN32_WINNT_WIN8
    #define POOL_NX_OPTIN 1
  #endif
#endif

namespace ktl {
namespace crt {
using pool_tag_t = uint32_t;
using pool_type_t = POOL_TYPE;

inline constexpr pool_tag_t DEFAULT_HEAP_TAG{'LTKd'},  // Reversed 'KTLd'
    KTL_HEAP_TAG{DEFAULT_HEAP_TAG};  //!< For back compatibility

inline constexpr size_t MEMORY_PAGE_SIZE{PAGE_SIZE}, CACHE_LINE_SIZE{64};

inline constexpr auto DEFAULT_ALLOCATION_ALIGNMENT{
    static_cast<std::align_val_t>(2 * sizeof(size_t))},  // 8 on x86, 16 on x64
    XMM_ALIGNMENT{static_cast<std::align_val_t>(16)},
    EXTENDED_ALLOCATION_ALIGNMENT{
        static_cast<std::align_val_t>(CACHE_LINE_SIZE)},  // cache line
    MAX_ALLOCATION_ALIGNMENT{static_cast<std::align_val_t>(MEMORY_PAGE_SIZE)};

void initialize_heap() noexcept;
}  // namespace crt

namespace heap::details {
template <class Request>
class heap_request_builder_base {
 public:
  using request_type = Request;
  using this_type = heap_request_builder_base<request_type>;

  constexpr heap_request_builder_base(request_type request)
      : m_request{request} {}

  constexpr this_type& set_alignment(std::align_val_t alignment) noexcept {
    m_request.alignment = alignment;
    return *this;
  }

  constexpr this_type& set_pool_tag(crt::pool_tag_t pool_tag) noexcept {
    m_request.pool_tag = pool_tag;
    return *this;
  }

  [[nodiscard]] constexpr request_type build() const noexcept {
    return m_request;
  }

 private:
  request_type m_request;
};
}  // namespace heap::details

ALIGN(crt::XMM_ALIGNMENT) struct alloc_request {
  size_t bytes_count;
  crt::pool_type_t pool_type;
  std::align_val_t alignment;
  crt::pool_tag_t pool_tag;
};

struct alloc_request_builder
    : heap::details::heap_request_builder_base<alloc_request> {
  using MyBase = heap_request_builder_base<alloc_request>;

  constexpr explicit alloc_request_builder(size_t count,
                                           crt::pool_type_t pool_type) noexcept
      : MyBase({count, pool_type}) {}
};

ALIGN(crt::XMM_ALIGNMENT)
struct free_request {
  void* memory_block;
  size_t bytes_count;
  std::align_val_t alignment;
  crt::pool_tag_t pool_tag;
};

struct free_request_builder
    : heap::details::heap_request_builder_base<free_request> {
  using MyBase = heap_request_builder_base<free_request>;

  constexpr explicit free_request_builder(void* memory_block,
                                          size_t bytes_count) noexcept
      : MyBase({memory_block, bytes_count}) {}
};

void* allocate_memory(alloc_request request) noexcept;
void deallocate_memory(free_request request) noexcept;
}  // namespace ktl
