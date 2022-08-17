#pragma once
#include <basic_types.hpp>
#include <crt_assert.hpp>

#include <ntddk.h>
#include <xmmintrin.h>

#if WINVER >= _WIN32_WINNT_WIN8
#define POOL_NX_OPTIN 1  // NOLINT(cppcoreguidelines-macro-usage)
#endif

namespace ktl {
namespace crt {
using pool_tag_t = uint32_t;
using pool_type_t = POOL_TYPE;

// clang-format off
// NOLINTNEXTLINE(clang-diagnostic-four-char-constants)
inline constexpr pool_tag_t DEFAULT_HEAP_TAG{'dLTK'};  //!< Reversed 'KTLd'
inline constexpr pool_tag_t KTL_HEAP_TAG{DEFAULT_HEAP_TAG};  //!< For back compatibility

inline constexpr size_t MEMORY_PAGE_SIZE{PAGE_SIZE};
inline constexpr size_t CACHE_LINE_SIZE{SYSTEM_CACHE_ALIGNMENT_SIZE};

inline constexpr auto XMM_ALIGNMENT{static_cast<align_val_t>(alignof(__m128))};
inline constexpr auto DEFAULT_ALLOCATION_ALIGNMENT{static_cast<align_val_t>(2 * sizeof(size_t))};  //!< 8 on x86, 16 on x64
inline constexpr auto CACHE_LINE_ALLOCATION_ALIGNMENT{static_cast<align_val_t>(CACHE_LINE_SIZE)};  //!< cache line
inline constexpr auto EXTENDED_ALLOCATION_ALIGNMENT{CACHE_LINE_ALLOCATION_ALIGNMENT};  //!< For backward compatibility
inline constexpr auto MAX_ALLOCATION_ALIGNMENT{static_cast<align_val_t>(MEMORY_PAGE_SIZE)};
// clang-format on

void initialize_heap() noexcept;
}  // namespace crt

namespace heap::details {
template <class Request, class ConcreteBuilder>
class heap_request_builder_base {
 public:
  using request_type = Request;

  constexpr heap_request_builder_base(request_type request)
      : m_request{request} {}

  constexpr ConcreteBuilder& set_alignment(
      std::align_val_t alignment) noexcept {
    m_request.alignment = alignment;
    return get_context();
  }

  constexpr ConcreteBuilder& set_pool_tag(crt::pool_tag_t pool_tag) noexcept {
    m_request.pool_tag = pool_tag;
    return get_context();
  }

  [[nodiscard]] constexpr request_type build() const noexcept {
    return m_request;
  }

 protected:
  constexpr ConcreteBuilder& get_context() noexcept {
    return static_cast<ConcreteBuilder&>(*this);
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
    : heap::details::heap_request_builder_base<alloc_request,
                                               alloc_request_builder> {
  using MyBase =
      heap_request_builder_base<alloc_request, alloc_request_builder>;

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
    : heap::details::heap_request_builder_base<free_request,
                                               free_request_builder> {
  using MyBase = heap_request_builder_base<free_request, free_request_builder>;

  constexpr explicit free_request_builder(void* memory_block,
                                          size_t bytes_count) noexcept
      : MyBase({memory_block, bytes_count}) {}
};

enum class OnAllocationFailure { DoNothing, ThrowException };

// pass-by-value using XMM registers

template <OnAllocationFailure OnFailure = OnAllocationFailure::DoNothing>
void* allocate_memory(alloc_request) noexcept(
    OnFailure != OnAllocationFailure::ThrowException);

extern template void* allocate_memory<OnAllocationFailure::DoNothing>(
    alloc_request request) noexcept;

extern template void* allocate_memory<OnAllocationFailure::ThrowException>(
    alloc_request request);

void deallocate_memory(free_request request) noexcept;
}  // namespace ktl
