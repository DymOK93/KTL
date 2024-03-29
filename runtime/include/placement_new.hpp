#pragma once
#include <basic_types.hpp>
#include <crt_attributes.hpp>

#if defined(KTL_NO_CXX_STANDARD_LIBRARY) && !defined(__PLACEMENT_NEW_INLINE)
void* CRTCALL operator new(size_t bytes_count, void* ptr) noexcept;
#endif