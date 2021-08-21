#include <new_delete.h>
#include <ntddk.h>

namespace ktl {
namespace mm::details {
static new_handler_t new_handler;
}

new_handler_t get_new_handler() noexcept {
  return static_cast<new_handler_t>(InterlockedCompareExchangePointer(
      reinterpret_cast<volatile PVOID*>(&mm::details::new_handler), nullptr,
      nullptr));
}

new_handler_t set_new_handler(new_handler_t new_h) noexcept {
  return static_cast<new_handler_t>(InterlockedExchangePointer(
      reinterpret_cast<volatile PVOID*>(&mm::details::new_handler), new_h));
}
}  // namespace ktl

void* CRTCALL operator new(size_t bytes_count) {
#ifdef KTL_USING_NON_PAGED_NEW_AS_DEFAULT
  return operator new (bytes_count, ktl::non_paged_new_tag_t{});
#else
  return operator new (bytes_count, ktl::paged_new_tag_t{});
#endif
}

void* CRTCALL operator new(size_t bytes_count, ktl::paged_new_tag_t tag) {
  return operator new(bytes_count, ktl::DEFAULT_NEW_ALIGNMENT, tag);
}

void* CRTCALL operator new(size_t bytes_count, ktl::non_paged_new_tag_t tag) {
  return operator new(bytes_count, ktl::DEFAULT_NEW_ALIGNMENT, tag);
}

void* CRTCALL operator new(size_t bytes_count, std::align_val_t alignment) {
#ifdef KTL_USING_NON_PAGED_NEW_AS_DEFAULT
  return operator new (bytes_count, alignment, ktl::non_paged_new_tag_t{});
#else
  return operator new (bytes_count, alignment, ktl::paged_new_tag_t{});
#endif
}

void* CRTCALL operator new(size_t bytes_count,
                           std::align_val_t alignment,
                           ktl::paged_new_tag_t) {
  return ktl::mm::details::operator_new_impl(
      [](size_t bytes_count, std::align_val_t alignment) noexcept {
        return ktl::alloc_paged(bytes_count, alignment);
      },
      bytes_count, alignment);
}

void* CRTCALL operator new(size_t bytes_count,
                           std::align_val_t alignment,
                           ktl::non_paged_new_tag_t) {
  return ktl::mm::details::operator_new_impl(
      [](size_t bytes_count, std::align_val_t alignment) noexcept {
        return ktl::alloc_non_paged(bytes_count, alignment);
      },
      bytes_count, alignment);
}

void* CRTCALL operator new(size_t bytes_count,
                           const nothrow_t& nthrw) noexcept {
#ifdef KTL_USING_NON_PAGED_NEW_AS_DEFAULT
  return operator new (bytes_count, nthrw, ktl::non_paged_new_tag_t{});
#else
  return operator new (bytes_count, nthrw, ktl::paged_new_tag_t{});
#endif
}

void* CRTCALL operator new(size_t bytes_count,
                           const nothrow_t& nthrw,
                           ktl::paged_new_tag_t tag) noexcept {
  return operator new(bytes_count, ktl::DEFAULT_NEW_ALIGNMENT, nthrw, tag);
}

void* CRTCALL operator new(size_t bytes_count,
                           const nothrow_t& nthrw,
                           ktl::non_paged_new_tag_t tag) noexcept {
  return operator new(bytes_count, ktl::DEFAULT_NEW_ALIGNMENT, nthrw, tag);
}

void* CRTCALL operator new(size_t bytes_count,
                           std::align_val_t alignment,
                           const nothrow_t& flag) noexcept {
#ifdef KTL_USING_NON_PAGED_NEW_AS_DEFAULT
  return operator new (bytes_count, alignment, flag,
                       ktl::non_paged_new_tag_t{});
#else
  return operator new (bytes_count, alignment, flag, ktl::paged_new_tag_t{});
#endif
}

void* CRTCALL operator new(size_t bytes_count,
                           std::align_val_t alignment,
                           const nothrow_t&,
                           ktl::paged_new_tag_t) noexcept {
  return ktl::alloc_paged(bytes_count, alignment);
}

void* CRTCALL operator new(size_t bytes_count,
                           std::align_val_t alignment,
                           const nothrow_t&,
                           ktl::non_paged_new_tag_t) noexcept {
  return ktl::alloc_non_paged(bytes_count, alignment);
}

void CRTCALL operator delete(void* ptr) noexcept {
  ktl::free(ptr);
}

void CRTCALL operator delete(void* ptr, ktl::paged_new_tag_t) noexcept {
  ktl::free(ptr);
}

void CRTCALL operator delete(void* ptr, ktl::non_paged_new_tag_t) noexcept {
  ktl::free(ptr);
}

void CRTCALL operator delete(void* ptr, std::align_val_t alignment) noexcept {
  ktl::free(ptr, alignment);
}

void CRTCALL operator delete(void* ptr,
                             std::align_val_t alignment,
                             ktl::paged_new_tag_t) noexcept {
  ktl::free(ptr, alignment);
}

void CRTCALL operator delete(void* ptr,
                             std::align_val_t alignment,
                             ktl::non_paged_new_tag_t) noexcept {
  ktl::free(ptr, alignment);
}

void CRTCALL operator delete(void* ptr, size_t bytes_count) noexcept {
  ktl::free(ptr, bytes_count);
}
void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             ktl::paged_new_tag_t) noexcept {
  ktl::free(ptr, bytes_count);
}
void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             ktl::non_paged_new_tag_t) noexcept {
  ktl::free(ptr, bytes_count);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             std::align_val_t alignment) noexcept {
  ktl::free(ptr, bytes_count, alignment);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             std::align_val_t alignment,
                             ktl::paged_new_tag_t) noexcept {
  ktl::free(ptr, bytes_count, alignment);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             std::align_val_t alignment,
                             ktl::non_paged_new_tag_t) noexcept {
  ktl::free(ptr, bytes_count, alignment);
}

void CRTCALL operator delete(void* ptr, const nothrow_t&) noexcept {
  ktl::free(ptr);
}

void CRTCALL operator delete(void* ptr,
                             const nothrow_t&,
                             ktl::paged_new_tag_t) noexcept {
  ktl::free(ptr);
}

void CRTCALL operator delete(void* ptr,
                             const nothrow_t&,
                             ktl::non_paged_new_tag_t) noexcept {
  ktl::free(ptr);
}

void CRTCALL operator delete(void* ptr,
                             std::align_val_t alignment,
                             const nothrow_t&) noexcept {
  ktl::free(ptr, alignment);
}

void CRTCALL operator delete(void* ptr,
                             std::align_val_t alignment,
                             const nothrow_t&,
                             ktl::paged_new_tag_t) noexcept {
  ktl::free(ptr, alignment);
}

void CRTCALL operator delete(void* ptr,
                             std::align_val_t alignment,
                             const nothrow_t&,
                             ktl::non_paged_new_tag_t) noexcept {
  ktl::free(ptr, alignment);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             const nothrow_t&) noexcept {
  ktl::free(ptr, bytes_count);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             const nothrow_t&,
                             ktl::paged_new_tag_t) noexcept {
  ktl::free(ptr, bytes_count);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             const nothrow_t&,
                             ktl::non_paged_new_tag_t) noexcept {
  ktl::free(ptr, bytes_count);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             std::align_val_t alignment,
                             const nothrow_t&) noexcept {
  ktl::free(ptr, bytes_count, alignment);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             std::align_val_t alignment,
                             const nothrow_t&,
                             ktl::paged_new_tag_t) noexcept {
  ktl::free(ptr, bytes_count, alignment);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             std::align_val_t alignment,
                             const nothrow_t&,
                             ktl::non_paged_new_tag_t) noexcept {
  ktl::free(ptr, bytes_count, alignment);
}