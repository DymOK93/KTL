#pragma once
#include <crt_runtime.h>

#if defined(_IA64_) || defined(_AMD64_)
// C initializers
#pragma section(".CRT$XIA", long, read)
CRTALLOC(".CRT$XIA") ktl::crt::handler_t __c_init_begin__[] = {nullptr};
#pragma section(".CRT$XIZ", long, read)
CRTALLOC(".CRT$XIZ") ktl::crt::handler_t __c_init_end__[] = {nullptr};
// C++ initializers
#pragma section(".CRT$XCA", long, read)
CRTALLOC(".CRT$XCA") ktl::crt::handler_t __cxx_ctors_begin__[] = {nullptr};
#pragma section(".CRT$XCZ", long, read)
CRTALLOC(".CRT$XCZ") ktl::crt::handler_t __cxx_ctors_end__[] = {nullptr};
#pragma data_seg()
#else
#pragma data_seg(".CRT$XIA")
CRTALLOC(".CRT$XIA") ktl::crt::handler_t __c_init_begin__[] = {nullptr};
#pragma data_seg(".CRT$XIZ")
CRTALLOC(".CRT$XIZ") ktl::crt::handler_t __c_init_end__[] = {nullptr};
#pragma data_seg(".CRT$XCA")
ktl::crt::handler_t __ctors_begin__[] = {nullptr};
#pragma data_seg(".CRT$XCZ")
ktl::crt::handler_t__ctors_end__[] = {nullptr};
#pragma data_seg()
#endif

#pragma data_seg(".STL$A")
ktl::crt::handler_t ___StlStartInitCalls__[] = {nullptr};
#pragma data_seg(".STL$L")
ktl::crt::handler_t ___StlEndInitCalls__[] = {nullptr};
#pragma data_seg(".STL$M")
ktl::crt::handler_t ___StlStartTerminateCalls__[] = {nullptr};
#pragma data_seg(".STL$Z")
ktl::crt::handler_t ___StlEndTerminateCalls__[] = {nullptr};
#pragma data_seg()