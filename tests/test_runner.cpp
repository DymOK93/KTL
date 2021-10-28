#include "test_runner.hpp"

using namespace ktl;

#ifdef FORCE_TERMINATE_ON_FAIL
#define EXIT_ON_FAIL(context)                   \
  ktl::crt::set_termination_context((context)); \
  ktl::terminate();
#define LOG_LEVEL DPFLTR_ERROR_LEVEL
#else
#define EXIT_ON_FAIL(context)
#define LOG_LEVEL DPFLTR_INFO_LEVEL
#endif

namespace test {
namespace details {
void print_impl(ansi_string_view msg) noexcept {
  DbgPrintEx(DPFLTR_DEFAULT_ID, LOG_LEVEL, "5Z\n", msg);
}
}  // namespace details

void check(bool cond, ansi_string_view hint) {
  check_equal(cond, true, hint);
}

Runner::~Runner() noexcept {
  if (m_fail_count > 0) {
    // details::print("{} unit tests failed\n", m_fail_count);
    EXIT_ON_FAIL(capture_failure_context())
  }
}

crt::termination_context Runner::capture_failure_context() noexcept {
  using namespace crt;
  return {BugCheckReason::AssertionFailure,
          reinterpret_cast<bugcheck_arg_t>(this),
          static_cast<bugcheck_arg_t>(m_fail_count)};
}
}  // namespace test
