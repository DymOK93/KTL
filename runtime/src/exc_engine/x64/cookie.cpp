#include <seh.hpp>
#include <symbol.hpp>

#pragma data_seg(".KTL$SECURITY")
CRTALLOC(".KTL$SECURITY")
const uintptr_t __security_cookie{0x00002B99'2DDFA232ull};
#pragma comment(linker, "/merge:.KTL$SECURITY=.data")