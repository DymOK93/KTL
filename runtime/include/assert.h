#pragma once
#include <ntddk.h>

namespace ktl::crt {
template <class Cond>
void assert_impl(const Cond& cond) {
  bool value{static_cast<bool>(cond)};
  NT_ASSERT(value);
}
}  // namespace ktl::crt

#ifdef KTL_RUNTIME_DBG
#define assert(cond) ktl::crt::assert_impl((cond))
#else
#define assert(cond)
#endif