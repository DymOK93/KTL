#pragma once
#include <basic_types.h>

#define CRTCALL __cdecl

#define EXTERN_C extern "C"
#define ALIGN(align_val) __declspec(align(align_val))
#define NOINLINE __declspec(noinline)

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#if __INTELLISENSE__
#define offsetof(type, member) ((size_t) & ((type*)0)->member)
#else
#define offsetof(type, member) __builtin_offsetof(type, member)
#endif

#define container_of(ptr, type, member) \
  reinterpret_cast<type*>((uintptr_t)(ptr)-offsetof(type, member))
