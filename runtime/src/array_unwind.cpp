#include <array_unwind.hpp>
#include <crt_attributes.hpp>

using namespace ktl;
using crt::array_handler_t;

#pragma warning(disable : 4297)
void CRTCALL __ehvec_ctor(void* arr_begin,
                          size_t element_size,
                          size_t count,
                          array_handler_t constructor,
                          array_handler_t destructor) {
  auto* current_obj{static_cast<byte*>(arr_begin)};
  size_t idx{0};

  try {
    for (; idx < count; ++idx) {
      constructor(current_obj);
      current_obj += element_size;
    }
  } catch (...) {
    __ehvec_dtor(current_obj, element_size, idx, destructor);
    throw;
  }
}
#pragma warning(default : 4297)

void CRTCALL __ehvec_dtor(void* arr_end,
                          size_t element_size,
                          size_t count,
                          array_handler_t destructor) {
  auto* current_obj{static_cast<byte*>(arr_end)};
  while (count-- > 0) {
    current_obj -= element_size;
    destructor(current_obj);
  }
}
