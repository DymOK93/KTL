#include "test.hpp"

#include <crt_attributes.hpp>

#include <ktlexcept.hpp>
#include <type_traits.hpp>

using namespace ktl;

EXTERN_C int _fltused;

namespace tests::floating_point {
namespace details {
template <class FloatingPoint,
          class ExpectedTy,
          class BinaryOp,
          enable_if_t<is_floating_point_v<FloatingPoint>, int> = 0>
void arithmetic_checker(FloatingPoint lhs,
                                 FloatingPoint rhs,
                                 ExpectedTy expected,
                                 BinaryOp op) {
  const FloatingPoint result{op(lhs, rhs)};

  const auto fp_expected{static_cast<FloatingPoint>(expected)};
  const auto delta{result - fp_expected};
  const FloatingPoint abs_delta{delta >= 0 ? delta : fp_expected - result};

  ASSERT_VALUE(abs_delta <= numeric_limits<FloatingPoint>::epsilon())
}

NOINLINE static void perform_arithmetic_operations_impl() {
  arithmetic_checker(3.5, -1.5, 2, plus<>{});
  arithmetic_checker(-33.0, 0.0, -33, plus<>{});
  arithmetic_checker(3.5, -1.5, 5, minus<>{});
  arithmetic_checker(-33.0, 0.0, -33, minus<>{});
}
}  // namespace details

static void validate_fltused() {
  ASSERT_VALUE(_fltused >= 0);
}

static void perform_arithmetic_operations() {
  XSTATE_SAVE state;

  const auto enabled_features{RtlGetEnabledExtendedFeatures(
      XSTATE_MASK_LEGACY | XSTATE_MASK_AVX | XSTATE_MASK_AVX512)};
  throw_exception_if_not<runtime_error>(
      enabled_features & XSTATE_MASK_LEGACY,
      "floating point operations are unsupported or disallowed by OS");

  NTSTATUS status{
      KeSaveExtendedProcessorState(enabled_features, addressof(state))};

  throw_exception_if_not<kernel_error>(
      NT_SUCCESS(status), status, "unable to save state of the FP-coprocessor");

  __try {
    details::perform_arithmetic_operations_impl();
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    status = GetExceptionCode();
  }
  KeRestoreExtendedProcessorState(addressof(state));

  throw_exception_if_not<kernel_error>(
      NT_SUCCESS(status), status,
      "floating point operation failed with SEH exception thrown");
}

void run_all(runner& runner) {
  RUN_TEST(runner, validate_fltused);
  RUN_TEST(runner, perform_arithmetic_operations);
}
}  // namespace tests::floating_point
