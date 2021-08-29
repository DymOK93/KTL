#include <crt_attributes.hpp>
#include <object_management.hpp>

using namespace ktl::crt;

EXTERN_C void CRTCALL __ehvec_ctor(void* arr_begin,
                                   size_t element_size,
                                   size_t count,
                                   object_handler_t constructor,
                                   object_handler_t destructor) {
  construct_array(arr_begin, element_size, count, constructor, destructor);
}

EXTERN_C void CRTCALL __ehvec_dtor(void* arr_end,
                                   size_t element_size,
                                   size_t count,
                                   object_handler_t destructor) {
  destroy_array_in_reversed_order(arr_end, element_size, count, destructor);
}
