#pragma once
#include <basic_types.h>

#include <cstddef>

#if defined(_M_IX86)
#define CRTCALL __cdecl
#define STDCALL __stdcall
#define FASTCALL __fastcall
#else
#define CRTCALL
#define STDCALL
#define FASTCALL
#endif

#ifndef VECTORCALL
#define VECTORCALL __vectorcall
#endif

#ifndef THISCALL
#define THISCALL __thiscall
#endif

#define EXTERN_C extern "C"
#define ALIGN(align_val) __declspec(align(align_val))
#define NOINLINE __declspec(noinline)

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#define STRINGIFY_IMPL(expr) #expr
#define STRINGIFY(expr) STRINGIFY_IMPL(expr)

#define IN_FILE_ON_LINE \
  CONCAT(CONCAT("in file ", __FILE__), CONCAT(" on line ", STRINGIFY(__LINE__)))

#define container_of(ptr, type, member) \
  reinterpret_cast<type*>((uintptr_t)(ptr)-offsetof(type, member))
