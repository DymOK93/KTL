#include <bugcheck.h>
#include <exception.h>
#include <thread.h>
#include <utility.hpp>

#include <ntddk.h>

namespace ktl {
namespace this_thread {
thread_id_t get_id() {
  return HandleToUlong(PsGetCurrentThreadId());
}

void yield() noexcept {
  YieldProcessor();
}
}  // namespace this_thread

namespace th::details {
thread_base::thread_base(internal_object_type thread_obj)
    : m_thread{thread_obj} {}

thread_base::thread_base(thread_base&& other) noexcept
    : m_thread{exchange(other.m_thread, native_handle_type{})} {}

thread_base& thread_base::operator=(thread_base&& other) noexcept {
  if (this != addressof(other)) {
    destroy();
    m_thread = exchange(other.m_thread, native_handle_type{});
  }
  return *this;
}

thread_base::~thread_base() noexcept {
  destroy();
}

void thread_base::swap(thread_base& other) noexcept {
  ktl::swap(m_thread, other.m_thread);
}

auto thread_base::get_id() const noexcept -> id {
  return HandleToUlong(PsGetThreadId(native_handle()));
}

uint32_t thread_base::hardware_concurrency() noexcept {
  return KeQueryActiveProcessorCount(nullptr);
}

void thread_base::destroy() noexcept {
  if (m_thread) {
    ObDereferenceObject(m_thread);
    m_thread = nullptr;
  }
}

auto thread_base::try_obtain_thread_object(HANDLE handle) noexcept
    -> internal_object_type {
  void* thread_obj;
  const NTSTATUS status{
      ObReferenceObjectByHandle(handle, THREAD_ALL_ACCESS, *PsThreadType,
                                KernelMode, addressof(thread_obj), nullptr)};
  return NT_SUCCESS(status) ? thread_obj : nullptr;
}
}  // namespace th::details

system_thread::~system_thread() noexcept {
  if (joinable()) {
    crt::set_termination_context({crt::BugCheckReason::DestroyingJoinableThread,
                                  get_id(), this_thread::get_id()});
    terminate();
  }
}

bool system_thread::joinable() const noexcept {
  return native_handle() != native_handle_type{};
}

void system_thread::join() {
  verify_joinable();
  KeWaitForSingleObject(native_handle(), Executive, KernelMode, false, nullptr);
  destroy();
}

void system_thread::detach() {
  verify_joinable();
  destroy();
}

void system_thread::verify_joinable() const {
  throw_exception_if_not<kernel_error>(joinable(), STATUS_INVALID_THREAD,
                                       L"thread isn't joinable",
                                       constexpr_message_tag{});
}

auto system_thread::create_thread_impl(thread_routine_t start, void* raw_args)
    -> internal_object_type {
  OBJECT_ATTRIBUTES attrs;
  InitializeObjectAttributes(addressof(attrs), nullptr, OBJ_KERNEL_HANDLE,
                             nullptr, nullptr);
  HANDLE thread_handle;
  const NTSTATUS status{PsCreateSystemThread(
      addressof(thread_handle), THREAD_ALL_ACCESS, addressof(attrs), nullptr,
      nullptr, start, raw_args)};
  throw_exception_if_not<kernel_error>(NT_SUCCESS(status), status,
                                       L"thread creation failed",
                                       constexpr_message_tag{});
  auto thread_obj{try_obtain_thread_object(thread_handle)};
  ZwClose(thread_handle);
  return thread_obj;
}

void system_thread::before_exit(NTSTATUS status) noexcept {
  PsTerminateSystemThread(status);
}

#if WINVER >= _WIN32_WINNT_WIN8
void io_thread::before_exit() noexcept {}

auto io_thread::create_thread_impl(void* io_object,
                                   thread_routine_t start,
                                   void* raw_args) -> internal_object_type {
  OBJECT_ATTRIBUTES attrs;
  InitializeObjectAttributes(addressof(attrs), nullptr, OBJ_KERNEL_HANDLE,
                             nullptr, nullptr);
  HANDLE thread_handle;
  const NTSTATUS status{IoCreateSystemThread(
      io_object, addressof(thread_handle), THREAD_ALL_ACCESS, addressof(attrs),
      nullptr, nullptr, start, raw_args)};
  throw_exception_if_not<kernel_error>(NT_SUCCESS(status), status,
                                       L"thread creation failed",
                                       constexpr_message_tag{});
  auto thread_obj{try_obtain_thread_object(thread_handle)};
  ZwClose(thread_handle);
  return thread_obj;
}
#endif
}  // namespace ktl