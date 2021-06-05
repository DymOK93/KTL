#include <../basic_types.h>
#include <bugcheck.h>
#include <cookie.h>

namespace ktl::crt::exc_engine {
EXTERN_C void FASTCALL __security_check_cookie(uintptr_t ptr) {
  if (ptr != __security_cookie || (ptr & KERNEL_MODE_ADDRESS_MASK) != 0) {
    set_termination_context({BugCheckReason::BufferSecurityCheckFailure});
    terminate();
  }
}

}  // namespace ktl::crt::exc_engine