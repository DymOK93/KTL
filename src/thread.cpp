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
thread_base::thread_base(internal_handle_type handle)
    : m_thread{obtain_thread_object(handle)} {}

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
  return HandleToUlong(PsGetThreadId(m_thread));
}

auto thread_base::native_handle() const noexcept -> native_handle_type {
  return m_thread;
}

uint32_t thread_base::hardware_concurrency() noexcept {
  return KeQueryActiveProcessorCount(nullptr);
}

void thread_base::destroy() const noexcept {
  if (m_thread) {
    ObDereferenceObject(m_thread);
  }
}

auto thread_base::obtain_thread_object(internal_handle_type handle)
    -> native_handle_type {
  void* thread_obj;
  const NTSTATUS status{
      ObReferenceObjectByHandle(handle, THREAD_ALL_ACCESS, *PsThreadType,
                                KernelMode, addressof(thread_obj), nullptr)};
  throw_exception_if_not<kernel_error>(
      NT_SUCCESS(status), status,
      L"unable to reference thread object by handle", constexpr_message_tag{});
  return static_cast<PETHREAD>(thread_obj);
}
}  // namespace th::details

void system_thread::before_exit(NTSTATUS status) noexcept {
  PsTerminateSystemThread(status);
}

void io_thread::before_exit() noexcept {}
}  // namespace ktl