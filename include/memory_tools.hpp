#pragma once
#include <heap.h>
#include <utility.hpp>

namespace winapi::kernel::mm {
template <class Ty, class... Types>
constexpr Ty* construct_at(Ty* place, Types&&... args) noexcept(
    is_nothrow_constructible_v<Ty, Types...>) {
  KdPrint(("Address: %p", static_cast<void*>(place)));
  return new (const_cast<void*>(static_cast<const volatile void*>(place)))
      Ty(forward<Types>(args)...);
}

template <class Ty>
constexpr void destroy_at(Ty* object_ptr) {
  object_ptr->~Ty();
}
}  // namespace winapi::kernel::mm