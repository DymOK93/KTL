#pragma once
#include <basic_types.h>

#include <ntddk.h>

namespace ktl::chrono {
using duration_t = uint64_t;

LARGE_INTEGER to_native_100ns_tics(duration_t period) noexcept {
  LARGE_INTEGER native_period;
  native_period.QuadPart = period;
  return native_period;
}
}  // namespace ktl::chrono