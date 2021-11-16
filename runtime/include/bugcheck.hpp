#pragma once
#include <basic_types.hpp>
#include <crt_attributes.hpp>
#include <irql.hpp>

#include <ntddk.h>

namespace ktl {
using terminate_handler_t = void (*)();

terminate_handler_t get_terminate() noexcept;
terminate_handler_t set_terminate(terminate_handler_t terminate) noexcept;

[[noreturn]] void terminate() noexcept;
[[noreturn]] void abort() noexcept;

namespace crt {
using bugcheck_code_t = uint32_t;
using bugcheck_arg_t = int64_t;

inline constexpr auto KTL_FAILURE{0xEA9B0000};  // 0xE, ('K' - 'A') % 10, ('T' -
                                                // 'A') %
                                                // 10, ('L' - 'A') % 10

enum class BugCheckReason : bugcheck_code_t {
  CorruptedPeHeader,

  UnwindOnUnsafeException,
  InvalidTargetUnwind,
  CorruptedFunctionUnwindState,
  CorruptedEhUnwindData,
  CorruptedExceptionState,

  UnwindingNonCxxFrame,

  SehHandlerNotInSafeseh,
  CorruptedMachineState,
  DestructorThrewDuringUnwind,
  CorruptedExceptionRegistrationChain,
  NoexceptViolation,
  ExceptionSpecificationNotSupported,
  CorruptedExceptionHandler,
  NoMatchingExceptionHandler,

  BufferSecurityCheckFailure,

  DestroyingJoinableThread,

  AssertionFailure,
  ForbiddenCall,
  StdAbort
};

struct termination_context {
  BugCheckReason reason;
  bugcheck_arg_t arg1{0}, arg2{0}, arg3{0}, arg4{0};
};

termination_context set_termination_context(
    const termination_context& bsod) noexcept;

EXTERN_C void verify_seh(NTSTATUS code,
                         const void* addr,
                         uint32_t flags) noexcept;

EXTERN_C void verify_seh_in_cxx_handler(NTSTATUS code,
                                        const void* addr,
                                        uint32_t flags,
                                        uint32_t unwind_info,
                                        const void* image_base) noexcept;

namespace details {
class termination_dispatcher {
 public:
  [[nodiscard]] terminate_handler_t get_handler() const noexcept;
  terminate_handler_t set_handler(terminate_handler_t handler) noexcept;

  termination_context set_context(const termination_context& bsod) noexcept;

  void terminate() const noexcept;
  [[noreturn]] void abort() const noexcept;

 private:
  void acquire_lock() const noexcept;
  void release_lock() const noexcept;

  static bugcheck_code_t to_bugcheck_code(BugCheckReason reason) noexcept;

 private:
  terminate_handler_t m_handler{nullptr};
  mutable long m_spin_lock{};
  mutable irql_t m_prev_irql;
  termination_context m_context{BugCheckReason::StdAbort};
};
}  // namespace details
}  // namespace crt
}  // namespace ktl
