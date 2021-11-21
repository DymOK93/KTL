#include <crt_attributes.hpp>

#include <chrono.hpp>
#include <string.hpp>

#include <modules/fmt/compile.hpp>
#include <modules/fmt/xchar.hpp>

#include <ntddk.h>

#include <runner/test_runner.hpp>

#include <dynamic_init/test.hpp>
#include <floating_point/test.hpp>
#include <heap/test.hpp>
#include <irql/test.hpp>
#include <preload_init/test.hpp>

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

  tests::dynamic_init::run_all(tr);
  tests::preload_init::run_all(tr);

  tests::floating_point::run_all(tr);

  tests::heap::run_all(tr);
  tests::irql::run_all(tr);
}