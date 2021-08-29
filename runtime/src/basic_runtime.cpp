#include <basic_runtime.hpp>
#include <object_management.hpp>
#include <preload_initializer.hpp>

namespace ktl::crt {
namespace details {
driver_context driver_ctx{};  // Non-static due to usage in mini-filter runtime library
}
}  // namespace ktl::crt

#define BREAK_ON_FAIL(status) \
  if (!NT_SUCCESS(status)) {  \
    break;                    \
  }

EXTERN_C NTSTATUS STDCALL
KtlDriverEntry(DRIVER_OBJECT* driver_object,
               UNICODE_STRING* registry_path) noexcept {
  ktl::crt::invoke_global_constructors();

  NTSTATUS init_status;
  do {
    init_status =
        ktl::crt::preload_initializer_registry::get_instance().run_all(
            *driver_object, *registry_path);
    BREAK_ON_FAIL(init_status)

    init_status = DriverEntry(driver_object, registry_path);
    BREAK_ON_FAIL(init_status)

    auto& drv_unload{driver_object->DriverUnload};
    if (auto& custom_drv_unload = ktl::crt::details::driver_ctx.custom_unload;
        !custom_drv_unload) {
      custom_drv_unload = drv_unload;
      drv_unload = &KtlDriverUnload;
    }

    return STATUS_SUCCESS;

  } while (false);
  KtlDriverUnload(driver_object);
  return init_status;
}

void STDCALL KtlDriverUnload(DRIVER_OBJECT* driver_object) noexcept {
  if (auto& drv_unload = ktl::crt::details::driver_ctx.custom_unload;
      drv_unload) {
    drv_unload(driver_object);
  }
  ktl::crt::invoke_global_destructors();
}

namespace ktl::crt {
DRIVER_OBJECT* get_driver_object() noexcept {
  return details::driver_ctx.object;
}
}  // namespace ktl::crt
