#include <../basic_types.hpp>
#include <bugcheck.hpp>

namespace ktl::crt::exc_engine {
EXTERN_C [[noreturn]] void report_security_check_failure() {
  set_termination_context({BugCheckReason::BufferSecurityCheckFailure});
  terminate();
}
}  // namespace ktl::crt::exc_engine