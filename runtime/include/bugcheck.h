#pragma once
#include <basic_types.h>
#include <crt_attributes.h>
#include <irql.h>

#include <ntddk.h>

namespace ktl {
using terminate_handler_t = void (*)();

terminate_handler_t get_terminate() noexcept;
terminate_handler_t set_terminate(terminate_handler_t terminate) noexcept;

[[noreturn]] void terminate() noexcept;
void terminate_if_not(bool cond) noexcept;

[[noreturn]] void critical_failure() noexcept;
[[noreturn]] void abort() noexcept;

namespace crt {
using bugcheck_code_t = uint32_t;
using bugcheck_arg_t = int64_t;

inline constexpr auto KTL_FAILURE{
    static_cast<bugcheck_code_t>(0xEA9B0000)};  // 0xE, ('K' - 'A') % 10, ('T' -
                                                // 'A') %
                                                // 10, ('L' - 'A') % 10

enum class BugCheckReason : bugcheck_code_t {
  UnwindOnUnsafeException,
  InvalidTargetUnwind,
  CorruptedFunctionUnwindState,
  CorruptedEhUnwindData,
  CorruptedExceptionState,

  UnwindingNonCxxFrame,

  SehHandlerNotInSafeseh,
  DestructorThrewDuringUnwind,
  CorruptedExceptionRegistrationChain,
  NoexceptViolation,
  ExceptionSpecificationNotSupported,
  NoMatchingExceptionHandler,

  AssertionFailure,
  ForbiddenCall,
  StdTerminate
};

bugcheck_code_t to_bucgcheck_code(BugCheckReason reason) noexcept;

struct Bsod {
  BugCheckReason reason;
  bugcheck_arg_t arg1{0}, arg2{0}, arg3{0}, arg4{0};
};

Bsod set_termination_context(const Bsod& bsod) noexcept;

[[noreturn]] void bugcheck(BugCheckReason reason) noexcept;
[[noreturn]] void bugcheck(const Bsod& bsod) noexcept;
[[noreturn]] void abort_impl() noexcept;
}  // namespace crt
}  // namespace ktl
