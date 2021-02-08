#pragma once
#include <heap.h>
#include <placement_new.h>
#include <exception_impl.hpp>

namespace ktl {
struct paged_new_tag_t {};
inline constexpr paged_new_tag_t paged_new;

struct non_paged_new_tag_t {};
inline constexpr non_paged_new_tag_t non_paged_new;

using new_handler_t = void (*)();

new_handler_t get_new_handler() noexcept;
new_handler_t set_new_handler(new_handler_t new_h) noexcept;

inline constexpr align_val_t DEFAULT_NEW_ALIGNMENT{
    crt::DEFAULT_ALLOCATION_ALIGNMENT};

namespace mm::details {
inline new_handler_t shared_new_handler;

template <class AllocFunc>
void* operator_new_impl(AllocFunc alloc_func,
                        size_t bytes_count,
                        align_val_t alignment) {
  void* allocated{nullptr};
  for (;;) {
    allocated = alloc_func(bytes_count, alignment);
    if (allocated) {
      break;
    } else if (auto new_handler = get_new_handler(); new_handler) {
      new_handler();
    } else {
      throw bad_alloc{};
    }
  }
  return allocated;
}
}  // namespace mm::details

}  // namespace ktl

void* CRTCALL operator new(size_t bytes);  // throw ktl::bad_alloc if fails
void* CRTCALL
operator new(size_t bytes,
             ktl::paged_new_tag_t);  // throw ktl::bad_alloc if fails
void* CRTCALL
operator new(size_t bytes,
             ktl::non_paged_new_tag_t);  // throw ktl::bad_alloc if fails

void* CRTCALL
operator new(size_t bytes,
             ktl::align_val_t alignment);  // throw ktl::bad_alloc if fails
void* CRTCALL
operator new(size_t bytes,
             ktl::align_val_t alignment,
             ktl::paged_new_tag_t);  // throw ktl::bad_alloc if fails
void* CRTCALL
operator new(size_t bytes,
             ktl::align_val_t alignment,
             ktl::non_paged_new_tag_t);  // throw ktl::bad_alloc if fails

void* CRTCALL operator new(size_t bytes, const nothrow_t&) noexcept;
void* CRTCALL operator new(size_t bytes,
                           const nothrow_t&,
                           ktl::paged_new_tag_t) noexcept;
void* CRTCALL operator new(size_t bytes,
                           const nothrow_t&,
                           ktl::non_paged_new_tag_t) noexcept;

void* CRTCALL operator new(size_t bytes,
                           ktl::align_val_t alignment,
                           const nothrow_t&) noexcept;
void* CRTCALL operator new(size_t bytes,
                           ktl::align_val_t alignment,
                           const nothrow_t&,
                           ktl::paged_new_tag_t) noexcept;
void* CRTCALL operator new(size_t bytes,
                           ktl::align_val_t alignment,
                           const nothrow_t&,
                           ktl::non_paged_new_tag_t) noexcept;

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             ktl::align_val_t alignment) noexcept;
void CRTCALL operator delete(void* ptr, size_t bytes_count) noexcept;
void CRTCALL operator delete(void* ptr) noexcept;