#include "object_management.h"

#include <crt_assert.h>
#include <crt_runtime.h>

#include <intrinsic.hpp>

namespace ktl::crt {
namespace details {
static handler_t destructor_stack[128] = {
    nullptr};  // C++ Standard requires 32 or more
static uint32_t destructor_count{0};
}  // namespace details

static constexpr unsigned int MAX_DESTRUCTOR_COUNT{
    sizeof(details::destructor_stack) / sizeof(handler_t)};

void CRTCALL invoke_constructors() noexcept {  //Вызов конструкторов
  for (ktl::crt::handler_t* ctor_ptr = __cxx_ctors_begin__;
       ctor_ptr < __cxx_ctors_end__; ++ctor_ptr) {
    auto& constructor{*ctor_ptr};
    if (constructor) {
      constructor();
    }
  }
}

void CRTCALL invoke_destructors() noexcept {
  auto& dtor_count{ktl::crt::details::destructor_count};
  for (uint32_t idx = dtor_count; idx > 0; --idx) {
    auto& destructor{ktl::crt::details::destructor_stack[idx - 1]};
    destructor();
  }
  dtor_count = 0;
}
}  // namespace ktl::crt

int CRTCALL atexit(ktl::crt::handler_t destructor) {
  if (auto& dtor_count = ktl::crt::details::destructor_count; destructor) {
    auto* idx_addr{reinterpret_cast<volatile long*>(&dtor_count)};
    const auto idx{static_cast<uint32_t>(InterlockedIncrement(idx_addr) - 1)};

    if (const bool registration_allowed = idx < ktl::crt::MAX_DESTRUCTOR_COUNT;
        registration_allowed) {
      ktl::crt::details::destructor_stack[idx] = destructor;
      return 0;
    }
    crt_assert_with_msg(
        false,
        "registered too many static objects with non-trivial destructors");
  }
  return 1;
}
