#pragma once
#include <basic_types.h>
#include <crt_attributes.h>

EXTERN_C void* CRTCALL memcpy(void* dst, const void* src, size_t size);
#pragma intrinsic(memcpy)

EXTERN_C void* CRTCALL memset(void* lhs, int pattern, size_t size);
#pragma intrinsic(memset)

EXTERN_C int CRTCALL memcmp(const void* lhs, const void* rhs, size_t size);
#pragma intrinsic(memcmp)

EXTERN_C unsigned char _BitScanForward(unsigned long* index,
                                       unsigned long mask);
#pragma intrinsic(_BitScanForward)

EXTERN_C unsigned char _BitScanForward64(unsigned long* index,
                                         unsigned long long mask);
#pragma intrinsic(_BitScanForward64)

EXTERN_C unsigned char _BitScanReverse(unsigned long* index,
                                       unsigned long mask);
#pragma intrinsic(_BitScanReverse)

EXTERN_C unsigned char _BitScanReverse64(unsigned long* index,
                                         unsigned long long mask);
#pragma intrinsic(_BitScanReverse64)

#if (BITNESS == 32)
#define BITSCANFORWARD _BitScanForward
#define BITSCANREVERSE _BitScanReverse
#else
#define BITSCANFORWARD _BitScanForward64
#define BITSCANREVERSE _BitScanReverse64
#endif

