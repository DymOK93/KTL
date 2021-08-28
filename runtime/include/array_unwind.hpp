#pragma once
#include <crt_attributes.hpp>

namespace ktl::crt {
using array_handler_t = void(CRTCALL*)(void*);
}

EXTERN_C void CRTCALL __ehvec_ctor(void* arr_begin,
                                   size_t element_size,
                                   size_t count,
                                   ktl::crt::array_handler_t constructor,
                                   ktl::crt::array_handler_t destructor);

EXTERN_C void CRTCALL
__ehvec_dtor(void* arr_end,
             size_t element_size,
             size_t count,
             ktl::crt::array_handler_t destructor);

