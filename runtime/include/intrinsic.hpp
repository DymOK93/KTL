#pragma once
#include <basic_types.h>
#include <crt_attributes.h>

EXTERN_C void* CRTCALL memcpy(void* dst, const void* src, size_t size);
#pragma intrinsic(memcpy)

EXTERN_C int CRTCALL memcmp(const void* lhs, const void* rhs, size_t size);
#pragma intrinsic(memcmp)

unsigned char _BitScanForward(uint32_t* Index, uint32_t Mask);
#pragma intrinsic(_BitScanForward)

unsigned char _BitScanForward64(uint32_t* Index, uint64_t Mask);
#pragma intrinsic(_BitScanForward64)

#if (BITNESS == 32)
#define BITSCANFORWARD() _BitScanForward
#else
#define BITSCANFORWARD _BitScanForward64
#endif



