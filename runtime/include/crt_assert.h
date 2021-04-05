#pragma once
#include <crt_attributes.h>
#include <ntddk.h>

namespace ktl::crt {
void assertion_handler(bool value, const wchar_t* description) noexcept;
void assertion_handler(bool value,
                       const wchar_t* description,
                       const wchar_t* msg) noexcept;

template <class Cond>
void assert_impl(const Cond& cond, const wchar_t* description) {
  assertion_handler(static_cast<bool>(cond), description);
}

template <class Cond>
void assert_impl(const Cond& cond,
                 const wchar_t* description,
                 const wchar_t* msg) {
  assertion_handler(static_cast<bool>(cond), description, msg);
}
}  // namespace ktl::crt

#define ASSERT_DESCRIPTION(cond) \
  CONCAT(L, CONCAT(CONCAT(STRINGIFY(cond), " "), IN_FILE_ON_LINE))

#define ASSERTION_CHECK(cond) \
  ktl::crt::assert_impl((cond), ASSERT_DESCRIPTION(cond))

#define ASSERTION_CHECK_WITH_MSG(cond, msg) \
  ktl::crt::assert_impl((cond), ASSERT_DESCRIPTION(cond), msg)

#ifdef KTL_RUNTIME_DBG
#define crt_assert(cond) ASSERTION_CHECK(cond)
#define crt_assert_with_msg(cond, msg) ASSERTION_CHECK_WITH_MSG(cond, (msg))
#else
#define crt_assert(cond)
#define crt_assert_with_msg(cond, msg)
#endif
