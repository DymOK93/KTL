#pragma once
#include <memory_tools.hpp>

#include <ntddk.h>
#include <type_traits>

namespace winapi::kernel::raii {
auto MakeHandleGuard(HANDLE handle) {
  auto guard{[](HANDLE target) { ObDereferenceObject(target); }};
  return std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(guard)>(handle,
                                                                         guard);
}
using handle_guard = std::invoke_result_t<decltype(MakeHandleGuard), HANDLE>;
}  // namespace winapi::kernel::raii