#if !defined(EX_NO_PUSH_LOCKS) && !defined(EX_LEGACY_PUSH_LOCKS)
#include <fltkernel.h>
#include <mutex.hpp>
#else
#include <ntddk.h>
#include <mutex.hpp>
#endif

namespace ktl {
#if !defined(EX_NO_PUSH_LOCKS) && !defined(EX_LEGACY_PUSH_LOCKS)
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

#else
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
#endif
}  // namespace ktl