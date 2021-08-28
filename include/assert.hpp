#pragma once
#include <crt_assert.hpp>

#ifdef KTL_CPP_DBG
#define assert(cond) ASSERTION_CHECK((cond))
#define assert_with_msg(cond, msg) ASSERTION_CHECK_WITH_MSG((cond), (msg))
#else
#define assert(cond)
#define assert_with_msg(cond, msg)
#endif