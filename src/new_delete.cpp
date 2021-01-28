#include <new_delete.h>
#include <ntddk.h>

namespace ktl {
new_handler get_new_handler() noexcept {
  return static_cast<new_handler>(InterlockedCompareExchangePointer(
      reinterpret_cast<volatile PVOID*>(&mm::details::shared_new_handler),
      nullptr, nullptr));
}

new_handler set_new_handler(new_handler new_h) noexcept {
  return static_cast<new_handler>(InterlockedExchangePointer(
      reinterpret_cast<volatile PVOID*>(&mm::details::shared_new_handler),
      new_h));
}
}  // namespace ktl

void* CRTCALL operator new(size_t bytes)  {
#ifdef KTL_USING_NON_PAGED_NEW_AS_DEFAULT
  return operator new (bytes, ktl::non_paged_new_tag_t{});
#else
  return operator new (bytes, ktl::paged_new_tag_t{});
#endif
}

void* CRTCALL operator new(size_t bytes, ktl::paged_new_tag_t) {
  return ktl::mm::details::operator_new_impl(bytes, &ktl::alloc_paged);
}

void* CRTCALL operator new(size_t bytes, ktl::non_paged_new_tag_t) {
  return ktl::mm::details::operator_new_impl(bytes, &ktl::alloc_non_paged);
}

void* CRTCALL operator new(size_t bytes, const nothrow_t& flag) noexcept {
#ifdef KTL_USING_NON_PAGED_NEW_AS_DEFAULT
  return operator new (bytes, flag, ktl::non_paged_new_tag_t{});
#else
  return operator new (bytes, flag, ktl::paged_new_tag_t{});
#endif
}

void* CRTCALL operator new(size_t bytes,
                           const nothrow_t&,
                           ktl::paged_new_tag_t) noexcept{
  return ktl::alloc_paged(bytes);
}

void* CRTCALL operator new(size_t bytes,
                           const nothrow_t&,
                           ktl::non_paged_new_tag_t) noexcept {
  return ktl::alloc_non_paged(bytes);
}

void CRTCALL operator delete(void* ptr, size_t bytes_count) {
  ktl::free(ptr, bytes_count);
}

void CRTCALL operator delete(void* ptr) {
  ktl::free(ptr);
}