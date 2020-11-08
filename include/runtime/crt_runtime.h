#pragma once
/**************************************************************************
See http://www.zer0mem.sk/?p=517
and http://www.hollistech.com/Resources/Cpp/kernel_c_runtime_library.htm
***************************************************************************/

#if defined(_IA64_) || defined(_AMD64_)
#pragma section(".CRT$XCA", long, read)
#pragma data_seg(".CRT$XCA")
__declspec(allocate(".CRT$XCA")) void (*__ctors_begin__[1])(void) = {nullptr};
#pragma section(".CRT$XCZ", long, read)
#pragma data_seg(".CRT$XCZ")
__declspec(allocate(".CRT$XCZ")) void (*__ctors_end__[1])(void) = {nullptr};
;
#pragma data_seg()
#else
#pragma data_seg(".CRT$XCA")
void (*__ctors_begin__[1])(void) = {nullptr};
#pragma data_seg(".CRT$XCZ")
void (*__ctors_end__[1])(void) = {nullptr};
#pragma data_seg()
#endif

#pragma data_seg(".STL$A")
void (*___StlStartInitCalls__[1])(void) = {0};
#pragma data_seg(".STL$L")
void (*___StlEndInitCalls__[1])(void) = {0};
#pragma data_seg(".STL$M")
void (*___StlStartTerminateCalls__[1])(void) = {0};
#pragma data_seg(".STL$Z")
void (*___StlEndTerminateCalls__[1])(void) = {0};
#pragma data_seg()

#if defined(_M_IA64) || defined(_M_AMD64)
#pragma comment(linker, "/merge:.CRT=.rdata")
#else
#pragma comment(linker, "/merge:.CRT=.data")
#endif