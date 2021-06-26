#pragma once
#include <../basic_types.h>

namespace ktl::crt::exc_engine {
struct symbol {
  operator const byte*() const noexcept {
    return reinterpret_cast<const byte*>(this);
  }

  operator uintptr_t() const noexcept {
    return reinterpret_cast<uintptr_t>(this);
  }
};
}