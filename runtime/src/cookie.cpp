#include <cookie.hpp>

#include <bugcheck.hpp>
#include <seh.hpp>
#include <symbol.hpp>

namespace ktl::crt {
#if BITNESS == 64
static constexpr uintptr_t DEFAULT_SECURITY_COOKIE{0x00002B99'2DDFA232ull};
#elif #if BITNESS == 32
static constexpr uintptr_t DEFAULT_SECURITY_COOKIE{0x0BB40E64Eul};
#else
#error Unsupported platform
#endif
}  // namespace ktl::crt

#pragma data_seg(".KTL_SECURITY")
CRTALLOC(".KTL_SECURITY")
EXTERN_C volatile uintptr_t __security_cookie{ktl::crt::DEFAULT_SECURITY_COOKIE};
#pragma comment(linker, "/merge:.KTL_SECURITY=.data")

namespace ktl::crt {
void verify_security_cookie() noexcept {
  if (__security_cookie == DEFAULT_SECURITY_COOKIE) {
    const termination_context bsod{
        BugCheckReason::BufferSecurityCheckFailure,
        reinterpret_cast<bugcheck_arg_t>(&__security_cookie),
        static_cast<bugcheck_arg_t>(__security_cookie),
        static_cast<bugcheck_arg_t>(DEFAULT_SECURITY_COOKIE)};
    set_termination_context(bsod);
    terminate();
  }
}
}  // namespace ktl::crt