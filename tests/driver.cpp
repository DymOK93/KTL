#include "dynamic_init/test.hpp"
#include "exception_dispatcher/test.hpp"
#include "floating_point/test.hpp"
#include "heap/test.hpp"
#include "irql/test.hpp"
#include "placement_new/test.hpp"
#include "preload_init/test.hpp"
#include "runner/test_runner.hpp"

#include <modules/fmt/compile.hpp>
#include <modules/fmt/xchar.hpp>

#include <chrono.hpp>
#include <crt_attributes.hpp>
#include <string.hpp>

#include <ntddk.h>

using namespace ktl;

TIME_FIELDS GetCurrentTime() noexcept;
void RunTests();

EXTERN_C NTSTATUS
DriverEntry([[maybe_unused]] DRIVER_OBJECT* driver_object,
            [[maybe_unused]] UNICODE_STRING* registry_path) noexcept {
  try {
    const auto time{GetCurrentTime()};
    auto str{
        format(FMT_COMPILE(L"[{:02}-{:02}-{:04} {:02}:{:02}:{:02}.{:03}][{}]"),
               time.Day, time.Month, time.Year, time.Hour, time.Minute,
               time.Second, time.Milliseconds, L"KTL Test Driver")};
    DbgPrint("%wZ\n", str.raw_str());

    RunTests();

  } catch (const exception& exc) {
    DbgPrint("Unhandled exception caught: %s with code %x\n", exc.what(),
             exc.code());
    return exc.code();
  }
  return STATUS_UNSUCCESSFUL;
}

TIME_FIELDS GetCurrentTime() noexcept {
  const auto current_time{
      chrono::system_clock::now().time_since_epoch().count()};
  LARGE_INTEGER native_time;
  native_time.QuadPart = static_cast<long long>(current_time);
  TIME_FIELDS time_fields;
  RtlTimeToTimeFields(addressof(native_time), addressof(time_fields));
  return time_fields;
}

void RunTests() {
  tests::runner tr;

  RUN_TEST(tr, tests::dynamic_init::verify_initializers);
  RUN_TEST(tr, tests::preload_init::verify_initializers);

  RUN_TEST(tr, tests::placement_new::construct_on_buffer);
  RUN_TEST(tr, tests::placement_new::construct_after_destroying);
  RUN_TEST(tr, tests::placement_new::construct_with_launder);

  RUN_TEST(tr, tests::floating_point::validate_fltused);
  RUN_TEST(tr, tests::floating_point::perform_arithmetic_operations);

  RUN_TEST(tr, tests::heap::alloc_and_free);
  RUN_TEST(tr, tests::heap::alloc_and_free_noexcept);

  RUN_TEST(tr, tests::irql::current);
  RUN_TEST(tr, tests::irql::raise_and_lower);
  RUN_TEST(tr, tests::irql::less_or_equal);

  RUN_TEST(tr, tests::exception_dispatcher::throw_directly);
  RUN_TEST(tr, tests::exception_dispatcher::throw_in_nested_call);
  RUN_TEST(tr, tests::exception_dispatcher::throw_on_array_init);
  RUN_TEST(tr, tests::exception_dispatcher::throw_in_new_expression);
  RUN_TEST(tr, tests::exception_dispatcher::throw_at_high_irql);
  RUN_TEST(tr, tests::exception_dispatcher::catch_by_value);
  RUN_TEST(tr, tests::exception_dispatcher::catch_by_base_ref);
  RUN_TEST(tr, tests::exception_dispatcher::catch_by_base_ptr);
}