#pragma once
#include <heap.h>
#include <placement_new.h>
#include <exception_impl.hpp>

namespace ktl {
struct paged_new_tag_t {};
inline constexpr paged_new_tag_t paged_new;

struct non_paged_new_tag_t {};
inline constexpr non_paged_new_tag_t non_paged_new;

using new_handler = void (*)();

new_handler get_new_handler() noexcept;
new_handler set_new_handler(new_handler new_h) noexcept;

namespace mm::details {
inline new_handler shared_new_handler;

template <class AllocFunc>
void* operator_new_impl(size_t bytes, AllocFunc alloc_func) {
  void* allocated{nullptr};
  for (;;) {
    allocated = alloc_func(bytes);
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

void* CRTCALL operator new(size_t bytes, const nothrow_t&) noexcept;
void* CRTCALL operator new(size_t bytes,
                           const nothrow_t&,
                           ktl::paged_new_tag_t) noexcept;
void* CRTCALL operator new(size_t bytes,
                           const nothrow_t&,
                           ktl::non_paged_new_tag_t) noexcept;

void CRTCALL operator delete(void* ptr, size_t bytes_count) noexcept;
void CRTCALL operator delete(void* ptr) noexcept;