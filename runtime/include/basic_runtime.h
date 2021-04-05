#pragma once
#include <crt_attributes.h>

#include <ntddk.h>

namespace ktl::crt {
using handler_t = void(CRTCALL*)(void);
using driver_unload_t = void(CRTCALL*)(PDRIVER_OBJECT);
}  // namespace ktl::crt

EXTERN_C int CRTCALL atexit(ktl::crt::handler_t destructor);

EXTERN_C NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object,
                              PUNICODE_STRING registry_path) noexcept;

EXTERN_C NTSTATUS KtlDriverEntry(PDRIVER_OBJECT driver_object,
                                 PUNICODE_STRING registry_path) noexcept;

EXTERN_C void KtlDriverUnload(PDRIVER_OBJECT driver_object) noexcept;
