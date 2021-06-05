#include <symbol.hpp>
#include <seh.hpp>

namespace ktl::crt::exc_engine {
EXTERN_C const uintptr_t __security_cookie = 0x00002B99'2DDFA232;
EXTERN_C const uintptr_t KERNEL_MODE_ADDRESS_MASK{0xFFFF'0000'0000'0000ull};
}  // namespace ktl::crt::exc_engine