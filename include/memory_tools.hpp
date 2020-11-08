#pragma once
#include <heap.h>
#include <memory>

namespace winapi::kernel::mm {
	
template <class Ty, class... Types>
constexpr Ty* construct_at(Ty* place, Types&&... args) {
  return ::new (const_cast<void*>(static_cast<const volatile void*>(place)))
      Ty(std::forward<Types>(args)...);
}

template <class Ty>
constexpr void destroy_at(Ty* object_ptr) {
  std::destroy_at(object_ptr);
}

}