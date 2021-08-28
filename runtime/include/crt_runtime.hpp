#pragma once
/**************************************************************************
See http://www.zer0mem.sk/?p=517
and http://www.hollistech.com/Resources/Cpp/kernel_c_runtime_library.htm
***************************************************************************/
#include <object_management.h>

#define CRTALLOC(x) __declspec(allocate(x))

#if defined(_IA64_) || defined(_AMD64_)
// C initializers
#pragma section(".CRT$XIA", long, read)
CRTALLOC(".CRT$XIA") inline ktl::crt::handler_t __c_init_begin__[] = {nullptr};
#pragma section(".CRT$XIZ", long, read)
CRTALLOC(".CRT$XIZ") inline ktl::crt::handler_t __c_init_end__[] = {nullptr};
// C++ initializers
#pragma section(".CRT$XCA", long, read)
CRTALLOC(".CRT$XCA")
inline ktl::crt::handler_t __cxx_ctors_begin__[] = {nullptr};
#pragma section(".CRT$XCZ", long, read)
CRTALLOC(".CRT$XCZ") inline ktl::crt::handler_t __cxx_ctors_end__[] = {nullptr};
#pragma data_seg()
#else
#pragma data_seg(".CRT$XIA")
inline ktl::crt::handler_t __c_init_begin__[] = {nullptr};
#pragma data_seg(".CRT$XIZ")
inline ktl::crt::handler_t __c_init_end__[] = {nullptr};
#pragma data_seg(".CRT$XCA")
inline ktl::crt::handler_t __ctors_begin__[] = {nullptr};
#pragma data_seg(".CRT$XCZ")
inline ktl::crt::handler_t__ctors_end__[] = {nullptr};
#pragma data_seg()
#endif

#pragma data_seg(".STL$A")
inline ktl::crt::handler_t ___StlStartInitCalls__[] = {nullptr};
#pragma data_seg(".STL$L")
inline ktl::crt::handler_t ___StlEndInitCalls__[] = {nullptr};
#pragma data_seg(".STL$M")
inline ktl::crt::handler_t ___StlStartTerminateCalls__[] = {nullptr};
#pragma data_seg(".STL$Z")
inline ktl::crt::handler_t ___StlEndTerminateCalls__[] = {nullptr};
#pragma data_seg()

#if defined(_M_IA64) || defined(_M_AMD64)
#pragma comment(linker, "/merge:.CRT=.rdata")
#else
#pragma comment(linker, "/merge:.CRT=.data")
#endif