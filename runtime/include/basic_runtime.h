#pragma once
#include <crt_attributes.h>

#include <ntddk.h>

namespace ktl::crt {
using handler_t = void(CRTCALL*)(void);
using driver_unload_t = void(CRTCALL*)(DRIVER_OBJECT*);
using init_routine_t = NTSTATUS (*)(void*, DRIVER_OBJECT&, UNICODE_STRING&);

struct initializer {
  init_routine_t handler{nullptr};
  void* context{nullptr};
};

bool inject_initializer(initializer init) noexcept;
}  // namespace ktl::crt

EXTERN_C int CRTCALL atexit(ktl::crt::handler_t destructor);

EXTERN_C NTSTATUS DriverEntry(DRIVER_OBJECT* driver_object,
                              UNICODE_STRING* registry_path) noexcept;

EXTERN_C NTSTATUS KtlDriverEntry(DRIVER_OBJECT* driver_object,
                                 UNICODE_STRING* registry_path) noexcept;

EXTERN_C void KtlDriverUnload(DRIVER_OBJECT* driver_object) noexcept;
