#pragma once
#include <crt_attributes.h>

#include <ntddk.h>

namespace ktl::crt {
using handler_t = void(CRTCALL*)(void);
using driver_unload_t = void(CRTCALL*)(DRIVER_OBJECT*);
}  // namespace ktl::crt

EXTERN_C int CRTCALL atexit(ktl::crt::handler_t destructor);

EXTERN_C NTSTATUS DriverEntry(DRIVER_OBJECT* driver_object,
                              UNICODE_STRING* registry_path) noexcept;

EXTERN_C NTSTATUS KtlDriverEntry(DRIVER_OBJECT* driver_object,
                                 UNICODE_STRING* registry_path) noexcept;

EXTERN_C void KtlDriverUnload(DRIVER_OBJECT* driver_object) noexcept;
