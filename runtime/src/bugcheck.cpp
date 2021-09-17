#include <ntddk.h>
#include <bugcheck.hpp>
#include <seh.hpp>

namespace ktl {
namespace crt::details {
static termination_dispatcher terminate_dispatcher;
}  // namespace crt::details

terminate_handler_t get_terminate() noexcept {
  return crt::details::terminate_dispatcher.get_handler();
}

terminate_handler_t set_terminate(terminate_handler_t terminate) noexcept {
  return crt::details::terminate_dispatcher.set_handler(terminate);
}

[[noreturn]] void terminate() noexcept {
  crt::details::terminate_dispatcher.terminate();
}

[[noreturn]] void abort() noexcept {
  crt::details::terminate_dispatcher.abort();
}

namespace crt {
termination_context set_termination_context(
    const termination_context& bsod) noexcept {
  return details::terminate_dispatcher.set_context(bsod);
}

void verify_seh(NTSTATUS code, const void* addr, uint32_t flags) noexcept {
  DbgPrintEx(
      DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL,
      "SEH exception caught with flag EXCEPTION_UNWIND! Code: 0x%08X, address: %p, flags: %u\n",
      code, addr, flags);
  if (const bool unwinding =
          flag_set<exc_engine::win::ExceptionFlag>{flags}.has_any_of(
              exc_engine::win::ExceptionFlag::Unwinding);
      unwinding) {
    set_termination_context({BugCheckReason::UnwindingNonCxxFrame, code,
                             reinterpret_cast<bugcheck_arg_t>(addr)});
    terminate();
  }
}

void verify_seh_in_cxx_handler(NTSTATUS code,
                               const void* addr,
                               uint32_t flags,
                               uint32_t unwind_info,
                               const void* image_base) noexcept {
  DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL,
             "SEH exception caught in CXX handler! Code: 0x%08X, address: %p, "
             "flags: %u, function "
             "unwind info offset: %u, image_base: %p)\n",
             code, addr, flags, unwind_info, image_base);
  if (const bool unwinding =
          flag_set<exc_engine::win::ExceptionFlag>{flags}.has_any_of(
              exc_engine::win::ExceptionFlag::Unwinding);
      unwinding) {
    set_termination_context({BugCheckReason::UnwindingNonCxxFrame, code,
                             reinterpret_cast<bugcheck_arg_t>(addr),
                             unwind_info,
                             reinterpret_cast<bugcheck_arg_t>(image_base)});
    terminate();
  }
}

namespace details {
terminate_handler_t termination_dispatcher::get_handler() const noexcept {
  auto* as_volatile{reinterpret_cast<const volatile PVOID*>(&m_handler)};
  auto* handler_ptr{const_cast<volatile PVOID*>(as_volatile)};
  const auto result{
      InterlockedCompareExchangePointer(handler_ptr, nullptr, nullptr)};
  return reinterpret_cast<terminate_handler_t>(result);
}

terminate_handler_t termination_dispatcher::set_handler(
    terminate_handler_t handler) noexcept {
  auto* as_volatile{reinterpret_cast<const volatile PVOID*>(&m_handler)};
  auto* handler_ptr{const_cast<volatile PVOID*>(as_volatile)};
  const auto prev{InterlockedExchangePointer(handler_ptr, handler)};
  return reinterpret_cast<terminate_handler_t>(prev);
}

termination_context termination_dispatcher::set_context(
    const termination_context& bsod) noexcept {
  acquire_lock();
  const termination_context old_context{m_context};
  m_context = bsod;
  release_lock();
  return old_context;
}

void termination_dispatcher::terminate() const noexcept {
  if (const auto terminate_handler = get_terminate(); terminate_handler) {
    terminate_handler();
  }
  abort();
}

[[noreturn]] void termination_dispatcher::abort() const noexcept {
  acquire_lock();
  const auto [reason, arg1, arg2, arg3, arg4]{m_context};
  KeBugCheckEx(KTL_FAILURE | to_bugcheck_code(reason), arg1, arg2, arg3, arg4);
}

void termination_dispatcher::acquire_lock() const noexcept {
  auto* lock_ptr{reinterpret_cast<volatile long*>(&m_spin_lock)};
  const irql_t prev_irql{raise_irql(HIGH_LEVEL)};
  while (InterlockedCompareExchange(lock_ptr, 1, 0)) {
    while (*lock_ptr) {
      YieldProcessor();
    }
  }
  m_prev_irql = prev_irql;
}

void termination_dispatcher::release_lock() const noexcept {
  auto& as_volatile{reinterpret_cast<volatile long&>(m_spin_lock)};
  const irql_t prev_irql{m_prev_irql};
  InterlockedExchange(&as_volatile, 0);
  lower_irql(prev_irql);
}

bugcheck_code_t termination_dispatcher::to_bugcheck_code(
    BugCheckReason reason) noexcept {
  return static_cast<bugcheck_code_t>(reason);
}

}  // namespace details
}  // namespace crt
}  // namespace ktl
