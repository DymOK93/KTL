#include <minifilter.hpp>

#include <basic_runtime.hpp>
#include <crt_assert.hpp>

namespace ktl::crt {
namespace details {
extern driver_context driver_ctx;
static filter_context filter_ctx{};
}
}  // namespace ktl::crt

EXTERN_C NTSTATUS STDCALL
KtlRegisterFilter(DRIVER_OBJECT* driver_object,
                  const FLT_REGISTRATION& flt_registration) noexcept {
  auto& drv_unload{driver_object->DriverUnload};
  ktl::crt::details::driver_ctx.custom_unload = drv_unload;
  drv_unload = &KtlDriverUnload;

  auto& filter{ktl::crt::details::filter_ctx.object};
  const NTSTATUS status{FltRegisterFilter(driver_object, &flt_registration, &filter)};
  if (!NT_SUCCESS(status)) {
    filter = nullptr;
    KdPrint(("Minifilter registration failed with status %#x", status));
  }
  return status;
}

EXTERN_C NTSTATUS STDCALL KtlStartFiltering() noexcept {
  auto* filter{ktl::crt::details::filter_ctx.object};
  crt_assert_with_msg(filter, "filter isn't registered");
  return FltStartFiltering(filter);
}

EXTERN_C void STDCALL KtlUnregisterFilter() noexcept {
  auto& filter{ktl::crt::details::filter_ctx.object};
  crt_assert_with_msg(filter, "filter isn't registered");
  FltUnregisterFilter(filter);
  filter = nullptr;
}

namespace ktl::crt {
PFLT_FILTER get_filter() noexcept {
  return details::filter_ctx.object;
}
}  // namespace ktl::crt