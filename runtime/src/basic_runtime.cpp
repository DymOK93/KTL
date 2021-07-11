#include <fltkernel.h>

#include <basic_types.h>
#include <crt_assert.h>
#include <crt_runtime.h>
#include <heap.h>
#include <object_management.h>
#include <preload_initializer.h>

EXTERN_C NTSTATUS STDCALL
DriverEntry([[maybe_unused]] DRIVER_OBJECT* driver_object,
            [[maybe_unused]] UNICODE_STRING* registry_path) noexcept;
#pragma comment(linker, "/alternatename:DriverEntry=DummyDriverEntry")

EXTERN_C NTSTATUS STDCALL
FilterEntry([[maybe_unused]] DRIVER_OBJECT* driver_object,
            [[maybe_unused]] UNICODE_STRING* registry_path,
            [[maybe_unused]] FLT_REGISTRATION& filter_registration) noexcept;
#pragma comment(linker, "/alternatename:FilterEntry=DummyFilterEntry")

static void STDCALL KtlDriverUnload(DRIVER_OBJECT* driver_object) noexcept;
static NTSTATUS STDCALL KtlFilterUnload(FLT_FILTER_UNLOAD_FLAGS flags) noexcept;

namespace ktl::crt {
constexpr NTSTATUS NO_START_FILTERING{STATUS_MORE_PROCESSING_REQUIRED};

using driver_unload_t = void(STDCALL*)(DRIVER_OBJECT*);
using filter_unload_t = NTSTATUS(STDCALL*)(FLT_FILTER_UNLOAD_FLAGS);

namespace details {
static DRIVER_OBJECT* driver_object{nullptr};
static driver_unload_t custom_driver_unload{nullptr};

static PFLT_FILTER filter{nullptr};
static filter_unload_t custom_filter_unload{nullptr};
}  // namespace details

static NTSTATUS install_filter(DRIVER_OBJECT* driver_object,
                               const FLT_REGISTRATION& filter_registration,
                               bool start_filtering) noexcept;
NTSTATUS try_start_filtering() noexcept;
static void force_filter_unload() noexcept;
void stop_filtering() noexcept;

}  // namespace ktl::crt

#define BREAK_ON_FAIL(status) \
  if (!NT_SUCCESS(status)) {  \
    break;                    \
  }

EXTERN_C NTSTATUS STDCALL
KtlDriverEntry(DRIVER_OBJECT* driver_object,
               UNICODE_STRING* registry_path) noexcept {
  ktl::crt::invoke_constructors();

  NTSTATUS init_status;
  do {
    init_status =
        ktl::crt::preload_initializer_registry::get_instance().run_all(
            *driver_object, *registry_path);
    BREAK_ON_FAIL(init_status)

    init_status = DriverEntry(driver_object, registry_path);
    BREAK_ON_FAIL(init_status)

    ktl::crt::details::custom_driver_unload = driver_object->DriverUnload;
    driver_object->DriverUnload = &KtlDriverUnload;
    ktl::crt::details::driver_object = driver_object;

    return STATUS_SUCCESS;

  } while (false);
  KtlDriverUnload(driver_object);
  return init_status;
}

void STDCALL KtlDriverUnload(DRIVER_OBJECT* driver_object) noexcept {
  if (auto& drv_unload = ktl::crt::details::custom_driver_unload; drv_unload) {
    drv_unload(driver_object);
  }
  ktl::crt::invoke_destructors();
}

EXTERN_C NTSTATUS STDCALL
KtlFilterEntry(DRIVER_OBJECT* driver_object,
               UNICODE_STRING* registry_path) noexcept {
  ktl::crt::invoke_constructors();

  NTSTATUS init_status;
  do {
    init_status =
        ktl::crt::preload_initializer_registry::get_instance().run_all(
            *driver_object, *registry_path);
    BREAK_ON_FAIL(init_status)

    FLT_REGISTRATION filter_registration{};
    init_status =
        FilterEntry(driver_object, registry_path, filter_registration);
    BREAK_ON_FAIL(init_status)

    ktl::crt::details::custom_driver_unload = driver_object->DriverUnload;
    driver_object->DriverUnload = &KtlDriverUnload;
    ktl::crt::details::driver_object = driver_object;

    ktl::crt::details::custom_filter_unload =
        filter_registration.FilterUnloadCallback;
    filter_registration.FilterUnloadCallback = &KtlFilterUnload;
    init_status =
        ktl::crt::install_filter(driver_object, filter_registration,
                                 init_status != ktl::crt::NO_START_FILTERING);
    BREAK_ON_FAIL(init_status)

    return STATUS_SUCCESS;

  } while (false);
  ktl::crt::force_filter_unload();
  return init_status;
}

NTSTATUS STDCALL KtlFilterUnload(FLT_FILTER_UNLOAD_FLAGS flags) noexcept {
  auto& filter{ktl::crt::details::filter};

  auto& flt_unload{ktl::crt::details::custom_filter_unload};
  NTSTATUS unload_status{
      flt_unload ? flt_unload(flags)
                 : STATUS_SUCCESS};  // filter in unloadable by default

  if (!filter) {
    crt_assert_with_msg(
        NT_SUCCESS(unload_status),
        L"can't return error status - filter was already unregistered");
    unload_status = STATUS_SUCCESS;
  } else if (NT_SUCCESS(unload_status)) {
    ktl::crt::stop_filtering();
  }
  return unload_status;
}

namespace ktl::crt {
NTSTATUS install_filter(DRIVER_OBJECT* driver_object,
                        const FLT_REGISTRATION& filter_registration,
                        bool start_filtering) noexcept {
  auto** filter_ptr{&details::filter};
  NTSTATUS flt_status{
      FltRegisterFilter(driver_object, &filter_registration,
                        filter_ptr)};  // FltMgr install it owns DriverUnload
  if (NT_SUCCESS(flt_status) && start_filtering) {
    flt_status = try_start_filtering();
    if (!NT_SUCCESS(flt_status)) {
      FltUnregisterFilter(*filter_ptr);
    }
  }
  return flt_status;
}

void force_filter_unload() noexcept {
  ktl::crt::invoke_destructors();
}

DRIVER_OBJECT* get_driver_object() noexcept {
  return details::driver_object;
}

NTSTATUS try_start_filtering() noexcept {
  crt_assert(details::filter);
  return FltStartFiltering(details::filter);
}

void stop_filtering() noexcept {
  auto& filter{ktl::crt::details::filter};
  crt_assert_with_msg(filter, L"filter isn't registered");
  FltUnregisterFilter(filter);
  filter = nullptr;
}
}  // namespace ktl::crt
