#include <crt_assert.h>

namespace ktl::crt {
void assertion_handler(const char description, const char msg) noexcept {
  const auto* fmt{!msg ? "Assertion failure: %s\n"
                       : "Assertion failure: %s\n%s\n"};
  DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, fmt, description, msg);
  DbgRaiseAssertionFailure();
}
}  // namespace ktl::crt