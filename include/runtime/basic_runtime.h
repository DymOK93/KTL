#pragma once
#include <crt_runtime.h>
#include <heap.h>

#include <ntddk.h>

#define CRTCALL _cdecl

namespace ktl::runtime {
using crt_handler_t = void(CRTCALL*)(void);
struct destroy_entry_t {
  destroy_entry_t* next{nullptr};
  crt_handler_t destructor;
};
inline destroy_entry_t* destructor_stack_head{};
}  // namespace ktl::runtime

extern "C" {
int CRTCALL atexit(ktl::runtime::crt_handler_t destructor) {
  if (destructor) {
    auto new_entry{new (nothrow) ktl::runtime::destroy_entry_t{
        ktl::runtime::destructor_stack_head, destructor}};
    if (new_entry) {
      ktl::runtime::destructor_stack_head = new_entry;
      return 1;
    }
  }
  return 0;
}

void CRTCALL doexit(_In_ int /*code*/,
                    _In_ int /*quick*/,
                    _In_ int /*retcaller*/
) {
  while (ktl::runtime::destructor_stack_head) {
    auto* target{ktl::runtime::destructor_stack_head};
    ktl::runtime::destructor_stack_head = target->next;
    target->destructor();
    delete target;
  }
}

int CRTCALL _cinit(_In_ int) {  //Вызов конструкторов
  for (void (**ctor)(void) = __ctors_begin__ + 1; ctor < __ctors_end__;
       ++ctor) {
    (*ctor)();
  }
  return 0;
}
}