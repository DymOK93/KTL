#include <new_delete.h>

void* CRTCALL operator new(size_t bytes, const nothrow_t&) noexcept {
#ifdef KTL_USING_NON_PAGED_NEW_AS_DEFAULT
    return ktl::alloc_non_paged(bytes);
#else
    return ktl::alloc_paged(bytes);
#endif
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