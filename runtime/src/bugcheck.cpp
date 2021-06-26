#include <bugcheck.h>
#include <ntddk.h>

namespace ktl {
namespace crt::details {
static terminate_handler_t terminate_handler{nullptr};
static KSPIN_LOCK bugcheck_guard{};
static Bsod bugcheck_context{
    BugCheckReason::StdAbort,
};
}  // namespace crt::details

terminate_handler_t get_terminate() noexcept {
  return static_cast<terminate_handler_t>(InterlockedCompareExchangePointer(
      reinterpret_cast<volatile PVOID*>(&crt::details::terminate_handler),
      nullptr, nullptr));
}

terminate_handler_t set_terminate(terminate_handler_t terminate) noexcept {
  return static_cast<terminate_handler_t>(InterlockedExchangePointer(
      reinterpret_cast<volatile PVOID*>(&crt::details::terminate_handler),
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

Bsod set_termination_context(const Bsod& bsod) noexcept {
  auto& lock{details::bugcheck_guard};
  irql_t prev_irql{get_current_irql()};

  if (prev_irql >= DISPATCH_LEVEL) {
    KeAcquireSpinLockAtDpcLevel(&lock);
  } else {
    KeAcquireSpinLock(&lock, &prev_irql);
  }

  auto& context{details::bugcheck_context};
  Bsod old_context{context};
  context = bsod;

  if (prev_irql >= DISPATCH_LEVEL) {
    KeReleaseSpinLockFromDpcLevel(&lock);
  } else {
    KeReleaseSpinLock(&lock, prev_irql);
  }

  return old_context;
}

[[noreturn]] void bugcheck(BugCheckReason reason) noexcept {
  bugcheck(Bsod{reason});
}

[[noreturn]] void bugcheck(const Bsod& bsod) noexcept {
  KeBugCheckEx(KTL_FAILURE | to_bucgcheck_code(bsod.reason), bsod.arg1,
               bsod.arg2, bsod.arg3, bsod.arg4);
}

[[noreturn]] void abort_impl() noexcept {
  if (auto& lock = details::bugcheck_guard;
      get_current_irql() >= DISPATCH_LEVEL) {
    KeAcquireSpinLockAtDpcLevel(&lock);
  } else {
    irql_t dummy;
    KeAcquireSpinLock(&lock, &dummy);
  }
  bugcheck(details::bugcheck_context);
}
}  // namespace crt
}  // namespace ktl
