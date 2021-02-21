#include <new_delete.h>
#include <ntddk.h>

namespace ktl {
new_handler_t get_new_handler() noexcept {
  return static_cast<new_handler_t>(InterlockedCompareExchangePointer(
      reinterpret_cast<volatile PVOID*>(&mm::details::shared_new_handler),
      nullptr, nullptr));
}

new_handler_t set_new_handler(new_handler_t new_h) noexcept {
  return static_cast<new_handler_t>(InterlockedExchangePointer(
      reinterpret_cast<volatile PVOID*>(&mm::details::shared_new_handler),
      new_h));
}
}  // namespace ktl

void* CRTCALL operator new(size_t bytes) {
#ifdef KTL_USING_NON_PAGED_NEW_AS_DEFAULT
  return operator new (bytes, ktl::non_paged_new_tag_t{});
#else
  return operator new (bytes, ktl::paged_new_tag_t{});
#endif
}

void* CRTCALL operator new(size_t bytes, ktl::paged_new_tag_t tag) {
  return operator new(bytes, ktl::DEFAULT_NEW_ALIGNMENT, tag);
}

void* CRTCALL operator new(size_t bytes, ktl::non_paged_new_tag_t tag) {
  return operator new(bytes, ktl::DEFAULT_NEW_ALIGNMENT, tag);
}

void* CRTCALL operator new(size_t bytes, std::align_val_t alignment) {
#ifdef KTL_USING_NON_PAGED_NEW_AS_DEFAULT
  return operator new (bytes, alignment, ktl::non_paged_new_tag_t{});
#else
  return operator new (bytes, alignment, ktl::paged_new_tag_t{});
#endif
}

void* CRTCALL operator new(size_t bytes,
                           std::align_val_t alignment,
                           ktl::paged_new_tag_t) {
  return ktl::mm::details::operator_new_impl(
      [](size_t bytes_count, std::align_val_t alignment) noexcept {
        return ktl::alloc_paged(bytes_count, alignment);
      },
      bytes, alignment);
}

void* CRTCALL operator new(size_t bytes,
                           std::align_val_t alignment,
                           ktl::non_paged_new_tag_t) {
  return ktl::mm::details::operator_new_impl(
      [](size_t bytes_count, std::align_val_t alignment) noexcept {
        return ktl::alloc_non_paged(bytes_count, alignment);
      },
      bytes, alignment);
}

void* CRTCALL operator new(size_t bytes, const nothrow_t& nthrw) noexcept {
#ifdef KTL_USING_NON_PAGED_NEW_AS_DEFAULT
  return operator new (bytes, nthrw, ktl::non_paged_new_tag_t{});
#else
  return operator new (bytes, nthrw, ktl::paged_new_tag_t{});
#endif
}

void* CRTCALL operator new(size_t bytes,
                           const nothrow_t& nthrw,
                           ktl::paged_new_tag_t tag) noexcept {
  return operator new(bytes, ktl::DEFAULT_NEW_ALIGNMENT, nthrw, tag);
}

void* CRTCALL operator new(size_t bytes,
                           const nothrow_t& nthrw,
                           ktl::non_paged_new_tag_t tag) noexcept {
  return operator new(bytes, ktl::DEFAULT_NEW_ALIGNMENT, nthrw, tag);
}

void* CRTCALL operator new(size_t bytes,
                           std::align_val_t alignment,
                           const nothrow_t& flag) noexcept {
#ifdef KTL_USING_NON_PAGED_NEW_AS_DEFAULT
  return operator new (bytes, alignment, flag, ktl::non_paged_new_tag_t{});
#else
  return operator new (bytes, alignment, flag, ktl::paged_new_tag_t{});
#endif
}

void* CRTCALL operator new(size_t bytes,
                           std::align_val_t alignment,
                           const nothrow_t&,
                           ktl::paged_new_tag_t) noexcept {
  return ktl::alloc_paged(bytes, alignment);
}

void* CRTCALL operator new(size_t bytes,
                           std::align_val_t alignment,
                           const nothrow_t&,
                           ktl::non_paged_new_tag_t) noexcept {
  return ktl::alloc_non_paged(bytes, alignment);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             std::align_val_t alignment) noexcept {
  ktl::free(ptr, bytes_count, alignment);
}

void CRTCALL operator delete(void* ptr, size_t bytes_count) {
  ktl::free(ptr, bytes_count);
}

void CRTCALL operator delete(void* ptr) {
  ktl::free(ptr);
}