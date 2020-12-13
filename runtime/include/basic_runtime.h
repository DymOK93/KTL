#pragma once
#include <ntddk.h>

#define CRTCALL __cdecl

namespace ktl::crt {
using handler_t = void(CRTCALL*)(void);
using driver_unload_t = void(CRTCALL*)(PDRIVER_OBJECT);
}  // namespace ktl::crt

extern "C" {
int CRTCALL atexit(ktl::crt::handler_t destructor);

NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object,
                     PUNICODE_STRING registry_path);

NTSTATUS KtlDriverEntry(PDRIVER_OBJECT driver_object,
                        PUNICODE_STRING registry_path);

void KtlDriverUnload(PDRIVER_OBJECT driver_object);
}
