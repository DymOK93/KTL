#pragma once
#include <fltkernel.h>

#include <crt_attributes.hpp>

namespace ktl::crt {
namespace details {
using filter_unload_t = NTSTATUS(STDCALL*)(FLT_FILTER_UNLOAD_FLAGS);
struct filter_context {
  PFLT_FILTER object{nullptr};
};
}  // namespace details

PFLT_FILTER get_filter() noexcept;
}  // namespace ktl::crt

EXTERN_C NTSTATUS STDCALL
KtlRegisterFilter(DRIVER_OBJECT* driver_object,
                  const FLT_REGISTRATION& flt_registration) noexcept;

EXTERN_C NTSTATUS STDCALL KtlStartFiltering() noexcept;
EXTERN_C void STDCALL KtlUnregisterFilter() noexcept;
