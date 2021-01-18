#pragma once

#define CRTCALL __cdecl
#define EXTERN_C extern "C"
#define ALIGN(align_val) __declspec(align(align_val))
#define NOINLINE __declspec(noinline)

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
