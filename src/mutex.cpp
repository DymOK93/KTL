#include <mutex.hpp>
#include <utility.hpp>

#include <ntddk.h>

namespace ktl::th::details {
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
}  // namespace ktl::th::details