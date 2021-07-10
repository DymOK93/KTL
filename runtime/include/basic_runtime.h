#pragma once
#ifdef KTL_MINIFILTER
#include <fltkernel.h>
#else
#include <ntddk.h>
#endif

#include <crt_assert.h>
#include <crt_attributes.h>

#ifndef KTL_MINIFILTER
EXTERN_C NTSTATUS STDCALL DriverEntry(DRIVER_OBJECT* driver_object,
                                      UNICODE_STRING* registry_path) noexcept;

EXTERN_C NTSTATUS STDCALL
KtlDriverEntry(DRIVER_OBJECT* driver_object,
               UNICODE_STRING* registry_path) noexcept;
#else
EXTERN_C NTSTATUS STDCALL
FilterEntry(DRIVER_OBJECT* driver_object,
            UNICODE_STRING* registry_path,
            FLT_REGISTRATION& filter_registration) noexcept;

EXTERN_C NTSTATUS STDCALL
KtlFilterEntry(DRIVER_OBJECT* driver_object,
               UNICODE_STRING* registry_path) noexcept;
#endif

namespace ktl::crt {
DRIVER_OBJECT* get_driver_object() noexcept;

#ifdef KTL_MINIFILTER
extern const NTSTATUS NO_START_FILTERING;
NTSTATUS try_start_filtering() noexcept;
void stop_filtering() noexcept;
#endif
}  // namespace ktl::crt
