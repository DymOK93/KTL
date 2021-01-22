#include <bugcheck.h>
#include <ntddk.h>

namespace ktl::crt {
bugcheck_code_t to_bucgcheck_code(BugCheckReason reason) noexcept {
  return static_cast<bugcheck_code_t>(reason);
}

[[noreturn]] void bugcheck(BugCheckReason reason) {
  bugcheck(Bsod{reason});
}

[[noreturn]] void bugcheck(const Bsod& bsod) {
  KeBugCheckEx(to_bucgcheck_code(bsod.reason), bsod.arg1, bsod.arg2, bsod.arg3,
               bsod.arg4);
}

[[noreturn]] void crt_critical_failure() {
  Bsod failure{BugCheckReason::StdTerminate, KTL_CRT_FAILURE};
  bugcheck(failure);
}

void crt_critical_failure_if_not(bool cond) {
  if (!cond) {
    crt_critical_failure();
  }
}
}  // namespace ktl::crt