#include "driver.hpp"
#include "test_runner.hpp"

#include <runtime/include/crt_attributes.hpp>

#include <include/chrono.hpp>
#include <include/string.hpp>
#include <include/unordered_map.hpp>
#include <include/unordered_set.hpp>

#include <modules/fmt/compile.hpp>
#include <modules/fmt/xchar.hpp>

#include <ntddk.h>

using namespace ktl;

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