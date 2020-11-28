#pragma once
#include <ntddk.h>
#include <smart_pointer.hpp>

namespace winapi::kernel::raii {
auto MakeHandleGuard(HANDLE handle) {
  return winapi::kernel::mm::unique_ptr(
      handle, [](HANDLE target) { ObDereferenceObject(target); });
}
using handle_guard = std::invoke_result_t<decltype(MakeHandleGuard), HANDLE>;
}  // namespace winapi::kernel::raii