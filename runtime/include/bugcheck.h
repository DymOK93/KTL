#pragma once

namespace ktl::crt {
using bugcheck_code_t = long;

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
  long arg1{0}, arg2{0}, arg3{0}, arg4{0};
};

[[noreturn]] void bugcheck(BugCheckReason reason);
[[noreturn]] void bugcheck(const Bsod& bsod);

}  // namespace ktl::crt