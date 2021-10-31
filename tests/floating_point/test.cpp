#include "test.hpp"

#include <crt_attributes.hpp>

#include <ktlexcept.hpp>
#include <limits.hpp>
#include <type_traits.hpp>

#include <test_runner.hpp>

using namespace ktl;

EXTERN_C int _fltused;

namespace test::floating_point {
namespace details {
template <class FloatingPoint,
          class BinaryOp,
          enable_if_t<is_floating_point_v<FloatingPoint>, int> = 0>
NOINLINE auto arithmetic_helper_impl(FloatingPoint lhs,
                                     FloatingPoint rhs,
                                     BinaryOp op) -> decltype(op(lhs, rhs)) {
  return op(lhs, rhs);
}

template <class FloatingPoint,
          class BinaryOp,
          enable_if_t<is_floating_point_v<FloatingPoint>, int> = 0>
auto arithmetic_helper(FloatingPoint lhs, FloatingPoint rhs, BinaryOp op)
    -> decltype(op(lhs, rhs)) {
  XSTATE_SAVE state;
  NTSTATUS status{
      KeSaveExtendedProcessorState(XSTATE_MASK_LEGACY, addressof(state))};
  throw_exception_if_not<kernel_error>(
      NT_SUCCESS(status), status, "unable to save state of the FP-coprocessor");

  decltype(op(lhs, rhs)) result{};
  __try {
    result = arithmetic_helper_impl(lhs, rhs, op);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    status = GetExceptionCode();
  }
  KeRestoreExtendedProcessorState(addressof(state));
  throw_exception_if_not<kernel_error>(
      NT_SUCCESS(status), status,
      "floating point operation failed with SEH exception thrown");

  return result;
}

template <class FloatingPoint,
          class ExpectedTy,
          class BinaryOp,
          enable_if_t<is_floating_point_v<FloatingPoint>, int> = 0>
void arithmetic_checker(FloatingPoint lhs,
                        FloatingPoint rhs,
                        ExpectedTy expected,
                        BinaryOp op) {
  const FloatingPoint result{arithmetic_helper(lhs, rhs, op)};

  const auto fp_expected{static_cast<FloatingPoint>(expected)};
  const auto delta{arithmetic_helper(result, fp_expected, minus<>{})};
  const FloatingPoint abs_delta{
      delta >= 0 ? delta : arithmetic_helper(fp_expected, result, minus<>{})};
  ASSERT_VALUE(arithmetic_helper(
      abs_delta, numeric_limits<FloatingPoint>::epsilon(),
      [](FloatingPoint fp, FloatingPoint eps) { return fp <= eps; }));
}
}  // namespace details

static void validate_fltused() {
  ASSERT_VALUE(_fltused >= 0);
}

static void perform_arithmetic_operations() {
  details::arithmetic_checker(3.5, -1.5, 2, plus<>{});
  details::arithmetic_checker(-33.0, 0.0, -33, plus<>{});
  details::arithmetic_checker(3.5, -1.5, 5, minus<>{});
  details::arithmetic_checker(-33.0, 0.0, -33, minus<>{});
}

void run_all(runner& runner) {
  RUN_TEST(runner, validate_fltused);
  RUN_TEST(runner, perform_arithmetic_operations);
}
}  // namespace test::floating_point
