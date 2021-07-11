#include <fltkernel.h>
#include <ntddk.h>

#include <crt_assert.h>
#include <crt_attributes.h>

EXTERN_C NTSTATUS STDCALL
DummyDriverEntry([[maybe_unused]] DRIVER_OBJECT* driver_object,
                 [[maybe_unused]] UNICODE_STRING* registry_path) noexcept {
  crt_assert_with_msg(false, L"invalid entry point");
  return STATUS_UNSUCCESSFUL;
}

EXTERN_C NTSTATUS STDCALL DummyFilterEntry(
    [[maybe_unused]] DRIVER_OBJECT* driver_object,
    [[maybe_unused]] UNICODE_STRING* registry_path,
    [[maybe_unused]] FLT_REGISTRATION& filter_registration) noexcept {
  crt_assert_with_msg(false, L"invalid entry point");
  return STATUS_UNSUCCESSFUL;
}
