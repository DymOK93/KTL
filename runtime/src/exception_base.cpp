#include <crt_attributes.h>
#include <exception_base.h>
#include <heap.h>


namespace ktl::crt {
using exception_index = uint16_t;
struct ExceptionList {
  static constexpr exception_index MAX_COUNT{1024};
  ExceptionBase* polymorphic_exc[MAX_COUNT];
};

ExceptionList& get_gobal_exception_list() {
  static ExceptionList exc_list{};
  return exc_list;
}

exception_index save_exception_in_global_list(ExceptionBase* exc_ptr) {
  exception_index idx{0};
  auto& exc_list{get_gobal_exception_list()};
  while (idx < exc_list.MAX_COUNT) {
    if (InterlockedCompareExchangePointer(
            reinterpret_cast<void**>(exc_list.polymorphic_exc + idx), exc_ptr,
            nullptr) == nullptr) {
      break;
    }
    ++idx;
  }
  return idx;
}

bool is_ktl_exception(NTSTATUS exc_code) {
  return exc_code & KTL_EXCEPTION;
}

exception_index extract_exception_index(NTSTATUS exc_code) {
  return static_cast<exception_index>(exc_code);
}

void init_exception_environment() {
  get_gobal_exception_list();
}

ExceptionBase* get_exception(NTSTATUS exc_code) {
  if (!is_ktl_exception(exc_code)) {
    return nullptr;
  }
  auto& exc_list{get_gobal_exception_list().polymorphic_exc};
  return exc_list[extract_exception_index(exc_code)];
}

void replace_exception(NTSTATUS old_exc_code, ExceptionBase* new_exc) {
  if (is_ktl_exception(old_exc_code)) {
    auto& exc_list{get_gobal_exception_list().polymorphic_exc};
    InterlockedExchangePointer(
        reinterpret_cast<void**>(exc_list +
                                 extract_exception_index(old_exc_code)),
        new_exc);
  }
}

void remove_exception(NTSTATUS exc_code) {
  replace_exception(exc_code, nullptr);
}

void cleanup_exception_environment() {
  auto& exc_list{get_gobal_exception_list().polymorphic_exc};
  for (auto* exc_ptr : exc_list) {
    delete exc_ptr;
  }
}

[[noreturn]] void throw_exception(ExceptionBase* exc_ptr) {
  if (auto exc_idx = save_exception_in_global_list(exc_ptr);
      exc_idx < get_gobal_exception_list().MAX_COUNT) {
    ExRaiseStatus(KTL_EXCEPTION | exc_idx);
  }
  delete exc_ptr;
  throw_ktl_crt_failure();
}

[[noreturn]] void throw_again(NTSTATUS exc_code) {
  ExRaiseStatus(exc_code);
}

[[noreturn]] void throw_ktl_crt_failure() {
  ExRaiseStatus(KTL_CRT_FAILURE);
}
}  // namespace ktl::crt

#define DEFINE_CXX_FRAME_HANDLER(number)                               \
  EXTERN_C disposition_t CONCAT(__CxxFrameHandler, number)(            \
      record_t * exc_rec, void* frame_ptr, cpu_context_t* cpu_context, \
      void* dispatcher_context) {                                      \
    return ktl::crt::exc_engine::handle_stack_frame(                   \
        exc_rec, frame_ptr, cpu_context, dispatcher_context);          \
  }

