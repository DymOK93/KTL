#pragma once
#include <crt_attributes.hpp>

namespace ktl::crt {
using global_handler_t = void(CRTCALL*)();
using object_handler_t = void(CRTCALL*)(void*);

void CRTCALL invoke_global_constructors() noexcept;
void CRTCALL invoke_global_destructors() noexcept;

void CRTCALL construct_array(void* arr_begin,
                             size_t element_size,
                             size_t count,
                             object_handler_t constructor,
                             object_handler_t destructor);

void CRTCALL destroy_array_in_reversed_order(void* arr_end,
                                             size_t element_size,
                                             size_t count,
                                             object_handler_t destructor);
}  // namespace ktl::crt
