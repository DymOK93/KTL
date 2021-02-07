#include <crt_assert.h>

namespace ktl::crt {
void assertion_handler(bool value, const wchar_t* description) noexcept {
  if (!value) {
    KdPrint(("Assertion failure: %ws\n", description));
    DbgRaiseAssertionFailure();
  }
}

void assertion_handler(bool value,
                    const wchar_t* description,
                    const wchar_t* msg) noexcept {
  if (!value) {
    KdPrint(("Assertion failure: %ws\n%ws\n", description, msg));
    DbgRaiseAssertionFailure();
  }
}
}  // namespace ktl::crt