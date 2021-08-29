#include <object_management.hpp>

#include <crt_assert.hpp>
#include <crt_runtime.hpp>

#include <intrinsic.hpp>

namespace ktl::crt {
namespace details {
static constexpr unsigned int MAX_DESTRUCTOR_COUNT{128}; // C++ Standard requires 32 or more
static global_handler_t destructor_stack[MAX_DESTRUCTOR_COUNT]{nullptr};
static uint32_t destructor_count{0};
}  // namespace details

void CRTCALL invoke_global_constructors() noexcept {  // Calling constructors of global objects during dynamic initialization
  for (global_handler_t* ctor_ptr = __cxx_ctors_begin__;    // ctor_ptr is always guaranteed to be non-null 
       ctor_ptr < __cxx_ctors_end__; ++ctor_ptr) {
    if (auto& constructor = *ctor_ptr; constructor) {
      constructor();
    }
  }
}

void CRTCALL invoke_global_destructors() noexcept {
  auto& dtor_count{details::destructor_count};
  for (uint32_t idx = dtor_count; idx > 0; --idx) {
    auto& destructor{details::destructor_stack[idx - 1]};
    destructor();
  }
  dtor_count = 0;
}

void CRTCALL construct_array(void* arr_begin,
                             size_t element_size,
                             size_t count,
                             object_handler_t constructor,
                             object_handler_t destructor) {
  auto* current_obj{static_cast<byte*>(arr_begin)};
  size_t idx{0};

  try {
    for (; idx < count; ++idx) {
      constructor(current_obj);
      current_obj += element_size;
    }
  } catch (...) {
    destroy_array_in_reversed_order(current_obj, element_size, idx, destructor);
    throw;
  }
}

void CRTCALL destroy_array_in_reversed_order(void* arr_end,
                                             size_t element_size,
                                             size_t count,
                                             object_handler_t destructor) {
  auto* current_obj{static_cast<byte*>(arr_end)};
  while (count-- > 0) {
    current_obj -= element_size;
    destructor(current_obj);
  }
}
}  // namespace ktl::crt

int CRTCALL atexit(ktl::crt::global_handler_t destructor) {
  using namespace ktl::crt;

  if (auto& dtor_count = details::destructor_count; destructor) {
    auto* idx_addr{reinterpret_cast<volatile long*>(&dtor_count)};
    const auto idx{static_cast<uint32_t>(InterlockedIncrement(idx_addr) - 1)};

    if (const bool registration_allowed = idx < details::MAX_DESTRUCTOR_COUNT;
        registration_allowed) {
      details::destructor_stack[idx] = destructor;
      return 0;
    }
    crt_assert_with_msg(
        false,
        "registered too many static objects with non-trivial destructors");
  }
  return 1;
}
