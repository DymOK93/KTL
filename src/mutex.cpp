#if !defined EX_NO_PUSH_LOCKS && defined(EX_LEGACY_PUSH_LOCKS)
#include <ntddk.h>
#else
#include <fltkernel.h>
#include <ntddk.h>
#endif

#include <ktlexcept.hpp>
#include <mutex.hpp>
#include <utility.hpp>

namespace ktl {
recursive_mutex::recursive_mutex() noexcept : MyBase() {
  KeInitializeMutex(native_handle(), 0);
}

void recursive_mutex::lock() noexcept {
  KeWaitForSingleObject(native_handle(),
                        Executive,   // Wait reason
                        KernelMode,  // Processor mode
                        false,
                        nullptr);  // Indefinite waiting
}

void recursive_mutex::unlock() noexcept {
  KeReleaseMutex(native_handle(), false);
}

fast_mutex::fast_mutex() noexcept : MyBase() {
  ExInitializeFastMutex(native_handle());
}

void fast_mutex::lock() noexcept {
  ExAcquireFastMutex(native_handle());
}

bool fast_mutex::try_lock() noexcept {
  return ExTryToAcquireFastMutex(native_handle());
}

void fast_mutex::unlock() noexcept {
  ExReleaseFastMutex(native_handle());
}

shared_mutex::shared_mutex() : MyBase() {
  const NTSTATUS status{ExInitializeResourceLite(native_handle())};
  throw_exception_if_not<kernel_error>(NT_SUCCESS(status), status,
                                       "initialization of ERESOURCE failed");
}

shared_mutex::~shared_mutex() noexcept {
  ExDeleteResourceLite(native_handle());
}

void shared_mutex::lock() noexcept {
  ExEnterCriticalRegionAndAcquireResourceExclusive(native_handle());
}

void shared_mutex::lock_shared() noexcept {
  ExEnterCriticalRegionAndAcquireResourceShared(native_handle());
}

void shared_mutex::unlock() noexcept {
  ExReleaseResourceAndLeaveCriticalRegion(native_handle());
}

void shared_mutex::unlock_shared() noexcept {
  ExReleaseResourceAndLeaveCriticalRegion(native_handle());
}

#ifndef KTL_EX_PUSH_LOCKS
push_lock::push_lock() noexcept : MyBase() {
  FltInitializePushLock(native_handle());
}

push_lock::~push_lock() noexcept {
  FltDeletePushLock(native_handle());
}

void push_lock::lock() {
  FltAcquirePushLockExclusive(native_handle());
}

void push_lock::lock_shared() {
  FltAcquirePushLockShared(native_handle());
}

void push_lock::unlock() {
  FltReleasePushLock(native_handle());
}

void push_lock::unlock_shared() {
  FltReleasePushLock(native_handle());
}
#else
push_lock::push_lock() noexcept : MyBase() {
  ExInitializePushLock(native_handle());
}

push_lock::~push_lock() noexcept = default;

void push_lock::lock() {
  ExAcquirePushLockExclusive(native_handle());
}

void push_lock::lock_shared() {
  ExAcquirePushLockShared(native_handle());
}

void push_lock::unlock() {
  ExReleasePushLockExclusive(native_handle());
}

void push_lock::unlock_shared() {
  ExReleasePushLockShared(native_handle());
}
#endif

namespace th::details {
void spin_lock_policy<SpinlockType::DpcOnly>::lock(
    KSPIN_LOCK& target) const noexcept {
  KeAcquireSpinLockAtDpcLevel(addressof(target));
}

bool spin_lock_policy<SpinlockType::DpcOnly>::try_lock(
    KSPIN_LOCK& target) const noexcept {
  return KeTryToAcquireSpinLockAtDpcLevel(addressof(target));
}

void spin_lock_policy<SpinlockType::DpcOnly>::unlock(
    KSPIN_LOCK& target) const noexcept {
  KeReleaseSpinLockFromDpcLevel(addressof(target));
}

void spin_lock_policy<SpinlockType::Mixed>::lock(
    KSPIN_LOCK& target) const noexcept {
  m_prev_irql = KeAcquireSpinLockRaiseToDpc(addressof(target));
}

bool spin_lock_policy<SpinlockType::Mixed>::try_lock(
    KSPIN_LOCK& target) const noexcept {
  const irql_t prev_irql{raise_irql(DISPATCH_LEVEL)};
  if (!KeTryToAcquireSpinLockAtDpcLevel(addressof(target))) {
    lower_irql(prev_irql);
    return false;
  }
  m_prev_irql = prev_irql;
  return true;
}

void spin_lock_policy<SpinlockType::Mixed>::unlock(
    KSPIN_LOCK& target) const noexcept {
  KeReleaseSpinLock(addressof(target), m_prev_irql);
}

void queued_spin_lock_policy<SpinlockType::DpcOnly>::lock(
    KSPIN_LOCK& target,
    KLOCK_QUEUE_HANDLE& queue_handle) const noexcept {
  KeAcquireInStackQueuedSpinLockAtDpcLevel(addressof(target),
                                           addressof(queue_handle));
}

void queued_spin_lock_policy<SpinlockType::DpcOnly>::unlock(
    KLOCK_QUEUE_HANDLE& queue_handle) const noexcept {
  KeReleaseInStackQueuedSpinLockFromDpcLevel(addressof(queue_handle));
}

void queued_spin_lock_policy<SpinlockType::Mixed>::lock(
    KSPIN_LOCK& target,
    KLOCK_QUEUE_HANDLE& queue_handle) const noexcept {
  KeAcquireInStackQueuedSpinLock(addressof(target), addressof(queue_handle));
}

void queued_spin_lock_policy<SpinlockType::Mixed>::unlock(
    KLOCK_QUEUE_HANDLE& queue_handle) const noexcept {
  KeReleaseInStackQueuedSpinLock(addressof(queue_handle));
}

event_base::event_base(event_type type, bool signaled) noexcept {
  KeInitializeEvent(native_handle(), type, signaled);
}

event_base& event_base::operator=(bool signaled) noexcept {
  if (!signaled) {
    KeClearEvent(native_handle());
  } else {
    KeSetEvent(native_handle(), 0, false);
  }
  return *this;
}

void event_base::wait() noexcept {
  wait_impl(native_handle(), nullptr);
}

void event_base::clear() noexcept {
  KeClearEvent(native_handle());
}

bool event_base::reset() noexcept {
  return KeResetEvent(native_handle()) != 0;
}

bool event_base::set(KPRIORITY priority_boost) noexcept {
  return KeSetEvent(native_handle(), priority_boost, false) != 0;
}

bool event_base::pulse(KPRIORITY priority_boost) noexcept {
  return KePulseEvent(native_handle(), priority_boost, false) != 0;
}

bool event_base::is_signaled() const noexcept {
  return KeReadStateEvent(get_non_const_native_handle()) != 0;
}

event_base::operator bool() const noexcept {
  return is_signaled();
}

cv_status event_base::from_native_status(NTSTATUS status) noexcept {
  return status == STATUS_TIMEOUT ? cv_status::timeout : cv_status::no_timeout;
}

NTSTATUS event_base::wait_impl(native_handle_type event,
                               const LARGE_INTEGER* timeout) noexcept {
  return KeWaitForSingleObject(
      event,
      Executive,   // Wait reason
      KernelMode,  // Processor mode
      false,
      const_cast<LARGE_INTEGER*>(timeout)  // Indefinite waiting
  );
}
}  // namespace th::details
}  // namespace ktl