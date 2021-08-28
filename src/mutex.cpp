#include <mutex.hpp>
#include <utility.hpp>

#include <ntddk.h>

namespace ktl {
namespace th::details {
void spin_lock_policy<SpinlockType::Apc>::lock(
    KSPIN_LOCK& target) const noexcept {
  KeAcquireSpinLock(addressof(target), addressof(m_prev_irql));
}

void spin_lock_policy<SpinlockType::Apc>::unlock(
    KSPIN_LOCK& target) const noexcept {
  KeReleaseSpinLock(addressof(target), m_prev_irql);
}

void spin_lock_policy<SpinlockType::Dpc>::lock(
    KSPIN_LOCK& target) const noexcept {
  KeAcquireSpinLockAtDpcLevel(addressof(target));
}

void spin_lock_policy<SpinlockType::Dpc>::unlock(
    KSPIN_LOCK& target) const noexcept {
  KeReleaseSpinLockFromDpcLevel(addressof(target));
}

void spin_lock_policy<SpinlockType::Mixed>::lock(
    KSPIN_LOCK& target) const noexcept {
  if (const irql_t irql = get_current_irql(); irql < DISPATCH_LEVEL) {
    KeAcquireSpinLock(addressof(target), addressof(m_prev_irql));
  } else {
    KeAcquireSpinLockAtDpcLevel(addressof(target));
    m_prev_irql = irql;
  }
}

void spin_lock_policy<SpinlockType::Mixed>::unlock(
    KSPIN_LOCK& target) const noexcept {
  if (m_prev_irql < DISPATCH_LEVEL) {
    KeReleaseSpinLock(addressof(target), m_prev_irql);
  } else {
    KeReleaseSpinLockFromDpcLevel(addressof(target));
  }
}

void queued_spin_lock_policy<SpinlockType::Apc>::lock(
    KSPIN_LOCK& target,
    KLOCK_QUEUE_HANDLE& queue_handle) const noexcept {
  KeAcquireInStackQueuedSpinLock(addressof(target), addressof(queue_handle));
}

void queued_spin_lock_policy<SpinlockType::Apc>::unlock(
    KLOCK_QUEUE_HANDLE& queue_handle) const noexcept {
  KeReleaseInStackQueuedSpinLock(addressof(queue_handle));
}

void queued_spin_lock_policy<SpinlockType::Dpc>::lock(
    KSPIN_LOCK& target,
    KLOCK_QUEUE_HANDLE& queue_handle) const noexcept {
  KeAcquireInStackQueuedSpinLockAtDpcLevel(addressof(target),
                                           addressof(queue_handle));
}

void queued_spin_lock_policy<SpinlockType::Dpc>::unlock(
    KLOCK_QUEUE_HANDLE& queue_handle) const noexcept {
  KeReleaseInStackQueuedSpinLockFromDpcLevel(addressof(queue_handle));
}

void queued_spin_lock_policy<SpinlockType::Mixed>::lock(
    KSPIN_LOCK& target,
    KLOCK_QUEUE_HANDLE& queue_handle) const noexcept {
  if (get_current_irql() < DISPATCH_LEVEL) {
    KeAcquireInStackQueuedSpinLock(addressof(target), addressof(queue_handle));
  } else {
    KeAcquireInStackQueuedSpinLockAtDpcLevel(addressof(target),
                                             addressof(queue_handle));
  }
}

void queued_spin_lock_policy<SpinlockType::Mixed>::unlock(
    KLOCK_QUEUE_HANDLE& queue_handle) const noexcept {
  if (queue_handle.OldIrql < DISPATCH_LEVEL) {
    KeReleaseInStackQueuedSpinLock(addressof(queue_handle));
  } else {
    KeReleaseInStackQueuedSpinLockFromDpcLevel(addressof(queue_handle));
  }
}
}  // namespace th::details
}  // namespace ktl