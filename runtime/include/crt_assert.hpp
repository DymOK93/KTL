#pragma once
#include <crt_attributes.hpp>
#include <ntddk.h>

namespace ktl::crt {
void assertion_handler(const char* description,
                       const char* msg = nullptr) noexcept;

template <class Cond>
constexpr void assert_impl(const Cond& cond,
                           const char* description,
                           const char* msg = nullptr) {
  if (!cond) {
    assertion_handler(description, msg);
  }
}
}  // namespace ktl::crt

#define ASSERT_DESCRIPTION(cond) \
  CONCAT(CONCAT(STRINGIFY(cond), " "), IN_FILE_ON_LINE)

#define ASSERTION_CHECK(cond) \
  ktl::crt::assert_impl((cond), ASSERT_DESCRIPTION(cond))

#define ASSERTION_CHECK_WITH_MSG(cond, msg) \
  ktl::crt::assert_impl((cond), ASSERT_DESCRIPTION(cond), msg)

#ifdef KTL_RUNTIME_DBG
#define crt_assert(cond) ASSERTION_CHECK((cond))
#define crt_assert_with_msg(cond, msg) ASSERTION_CHECK_WITH_MSG((cond), (msg))
#else
#define crt_assert(cond)
#define crt_assert_with_msg(cond, msg)
#endif
