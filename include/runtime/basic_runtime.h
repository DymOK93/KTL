#pragma once
#include <crt_runtime.h>
#include <heap.h>

#include <ntddk.h>

namespace winapi::kernel::runtime {
using crt_handler_t = void(__cdecl*)(void);
struct destroy_entry_t {
  destroy_entry_t* next{nullptr};
  crt_handler_t destructor;
};
inline destroy_entry_t* destructor_stack_head{};
}  // namespace winapi::kernel::runtime

extern "C" {
int __cdecl atexit(winapi::kernel::runtime::crt_handler_t destructor) {
  namespace runtime = winapi::kernel::runtime;
  if (destructor) {
    auto new_entry{new (std::nothrow) runtime::destroy_entry_t{
        runtime::destructor_stack_head, destructor}};
    if (new_entry) {
      runtime::destructor_stack_head = new_entry;
      return 1;
    }
  }
  return 0;
}

void __cdecl doexit(_In_ int /*code*/,
                    _In_ int /*quick*/,
                    _In_ int /*retcaller*/
) {
  namespace runtime = winapi::kernel::runtime;
  while (runtime::destructor_stack_head) {
    auto* target{runtime::destructor_stack_head};
    runtime::destructor_stack_head = target->next;
    target->destructor();  //Нужен ли здесь этот вызов?
    delete target;
  }
}

int __cdecl _cinit(_In_ int) {  //Вызов конструкторов
  for (void (**ctor)(void) = __ctors_begin__ + 1; ctor < __ctors_end__;
       ++ctor) {
    (*ctor)();
  }
  return 0;
}
}