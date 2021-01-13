#include <placement_new.h>

#if defined(KTL_NO_CXX_STANDARD_LIBRARY) && !defined(__PLACEMENT_NEW_INLINE)
void* CRTCALL operator new(size_t bytes_count, void* ptr) noexcept {
  (void)bytes_count;
  return ptr;
}
#endif