#include <basic_runtime.h>
#include <basic_types.h>
#include <crt_runtime.h>
#include <heap.h>
#include <preload_initializer.h>

namespace ktl::crt {
namespace details {
static handler_t destructor_stack[128] = {
    nullptr};  // C++ Standard requires 32 or more
static uint32_t destructor_count{0};
static driver_unload_t custom_driver_unload{nullptr};
}  // namespace details

static constexpr unsigned int MAX_DESTRUCTOR_COUNT{
    sizeof(details::destructor_stack) / sizeof(handler_t)};

static constexpr int INIT_CODE = 0x4b544caU;
static constexpr int EXIT_CODE = 0x4b544ceU;

void CRTCALL doexit(_In_ int) noexcept {
  auto& dtor_count{ktl::crt::details::destructor_count};
  for (uint32_t idx = dtor_count; idx > 0; --idx) {
    auto& destructor{ktl::crt::details::destructor_stack[idx - 1]};
    destructor();
  }
  dtor_count = 0;
}

int CRTCALL cinit(_In_ int) noexcept {  //����� �������������
  for (ktl::crt::handler_t* ctor_ptr = __cxx_ctors_begin__;
       ctor_ptr < __cxx_ctors_end__; ++ctor_ptr) {
    auto& constructor{*ctor_ptr};
    if (constructor) {
      constructor();
    }
  }
  return 0;
}
}  // namespace ktl::crt

int CRTCALL atexit(ktl::crt::handler_t destructor) {
  if (auto& dtor_count = ktl::crt::details::destructor_count;
      destructor && dtor_count < ktl::crt::MAX_DESTRUCTOR_COUNT) {
    ktl::crt::details::destructor_stack[dtor_count++] = destructor;
    return 0;
  }
  return 1;
}

NTSTATUS KtlDriverEntry(DRIVER_OBJECT* driver_object,
                        UNICODE_STRING* registry_path) noexcept {
  ktl::crt::cinit(ktl::crt::INIT_CODE);
  NTSTATUS init_status{
      ktl::crt::preload_initializer_registry::get_instance().run_all(
          *driver_object, *registry_path)};
  if (!NT_SUCCESS(init_status)) {
    KtlDriverUnload(driver_object);
  } else {
    init_status = DriverEntry(driver_object, registry_path);
    if (!NT_SUCCESS(init_status)) {
      KtlDriverUnload(driver_object);
    } else {
      ktl::crt::details::custom_driver_unload = driver_object->DriverUnload;
      driver_object->DriverUnload = &KtlDriverUnload;
    }
  }
  return init_status;
}

void KtlDriverUnload(PDRIVER_OBJECT driver_object) noexcept {
  if (auto& drv_unload = ktl::crt::details::custom_driver_unload; drv_unload) {
    drv_unload(driver_object);
  }
  ktl::crt::doexit(ktl::crt::EXIT_CODE);
}