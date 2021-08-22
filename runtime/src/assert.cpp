#include <crt_assert.h>

namespace ktl::crt {
void assertion_handler(const wchar_t* description,
                       const wchar_t* msg) noexcept {
  const auto* fmt{!msg ? "Assertion failure: %ws\n"
                      : "Assertion failure: %ws\n%ws\n"};
  DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, fmt, description, msg);
  DbgRaiseAssertionFailure();
}
}  // namespace ktl::crt