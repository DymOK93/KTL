#pragma once
#include <ntddk.h>
#include <smart_pointer.hpp>

namespace ktl::raii {
auto MakeHandleGuard(HANDLE handle) {
  return unique_ptr(
      handle, [](HANDLE target) { ObDereferenceObject(target); });
}
using handle_guard = std::invoke_result_t<decltype(MakeHandleGuard), HANDLE>;
}  // namespace ktl::raii