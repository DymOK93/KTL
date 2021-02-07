#pragma once
#include <basic_types.h>
#include <crt_attributes.h>

namespace ktl::crt {
using bugcheck_code_t = uint32_t;
using bugcheck_arg_t = int64_t;

inline constexpr auto KTL_FAILURE_BASE{
    static_cast<bugcheck_code_t>(0xEA9B0000)};  // 0xE, ('K' - 'A') % 10, ('T' -
                                                // 'A') %
                                                // 10, ('L' - 'A') % 10

inline constexpr auto KTL_CRT_FAILURE{
    static_cast<bugcheck_code_t>(KTL_FAILURE_BASE | 0xFFFF)};

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
  StdTerminate,
};

bugcheck_code_t to_bucgcheck_code(BugCheckReason reason) noexcept;

struct Bsod {
  BugCheckReason reason;
  bugcheck_arg_t arg1{0}, arg2{0}, arg3{0}, arg4{0};
};

[[noreturn]] void bugcheck(BugCheckReason reason);
[[noreturn]] void bugcheck(const Bsod& bsod);
[[noreturn]] void crt_critical_failure();
void crt_critical_failure_if_not(bool cond);
}  // namespace ktl::crt

EXTERN_C [[noreturn]] void terminate();