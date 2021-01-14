#pragma once
#include <heap.h>
#include <placement_new.h>

namespace ktl {
struct paged_new_tag_t {};
inline constexpr paged_new_tag_t paged_new;

struct non_paged_new_tag_t {};
inline constexpr non_paged_new_tag_t non_paged_new;
}  // namespace ktl

//Защита на случай подключения стандартного хедера <memory>
#if defined(KTL_NO_CXX_STANDARD_LIBRARY) && !defined(__PLACEMENT_NEW_INLINE)
void* CRTCALL operator new(size_t bytes_count, void* ptr) noexcept;
#endif
void* CRTCALL operator new(size_t bytes,
                           const nothrow_t&,
                           ktl::non_paged_new_tag_t) noexcept;
void* CRTCALL operator new(size_t bytes, const nothrow_t&) noexcept;
void CRTCALL operator delete(void* ptr, size_t bytes_count);
void CRTCALL operator delete(void* ptr);