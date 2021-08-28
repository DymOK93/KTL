#pragma once
#include <ntddk.h>

#include <crt_assert.hpp>
#include <crt_attributes.hpp>

namespace ktl::crt {
namespace details {
using driver_unload_t = void(STDCALL*)(DRIVER_OBJECT*);
struct driver_context {
  DRIVER_OBJECT* object{nullptr};
  driver_unload_t custom_unload{nullptr};
};
}  // namespace details

DRIVER_OBJECT* get_driver_object() noexcept;
}  // namespace ktl::crt

EXTERN_C NTSTATUS STDCALL DriverEntry(DRIVER_OBJECT* driver_object,
                                      UNICODE_STRING* registry_path) noexcept;

EXTERN_C NTSTATUS STDCALL
KtlDriverEntry(DRIVER_OBJECT* driver_object,
               UNICODE_STRING* registry_path) noexcept;

EXTERN_C void STDCALL KtlDriverUnload(DRIVER_OBJECT* driver_object) noexcept;
