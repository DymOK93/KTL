#include <bugcheck.h>
#include <ntddk.h>

namespace ktl {
terminate_handler_t get_terminate() noexcept {
  return static_cast<terminate_handler_t>(InterlockedCompareExchangePointer(
      reinterpret_cast<volatile PVOID*>(
          &crt::details::shared_terminate_handler),
      nullptr, nullptr));
}

terminate_handler_t set_terminate(terminate_handler_t terminate) noexcept {
  return static_cast<terminate_handler_t>(
      InterlockedExchangePointer(reinterpret_cast<volatile PVOID*>(
                                     &crt::details::shared_terminate_handler),
                                 terminate));
}

[[noreturn]] void terminate() noexcept {
  critical_failure();
}

void terminate_if_not(bool cond) noexcept {
  if (!cond) {
    critical_failure();
  }
}

[[noreturn]] void critical_failure() noexcept {
  if (auto terminate_handler = get_terminate(); terminate_handler) {
    terminate_handler();
  }
  abort();
}

[[noreturn]] void abort() noexcept {
  crt::abort_impl();
}

namespace crt {
bugcheck_code_t to_bucgcheck_code(BugCheckReason reason) noexcept {
  return static_cast<bugcheck_code_t>(reason);
}

[[noreturn]] void bugcheck(BugCheckReason reason) noexcept {
  bugcheck(Bsod{reason});
}

[[noreturn]] void bugcheck(const Bsod& bsod) noexcept {
  KeBugCheckEx(to_bucgcheck_code(bsod.reason), bsod.arg1, bsod.arg2, bsod.arg3,
               bsod.arg4);
}

[[noreturn]] void abort_impl() noexcept {
  Bsod failure{BugCheckReason::StdTerminate, KTL_CRT_FAILURE};
  bugcheck(failure);
}
}  // namespace crt
}  // namespace ktl
