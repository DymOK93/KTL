#include <fltkernel.h>
#include <ntddk.h>

#if !defined EX_NO_PUSH_LOCKS && !defined(EX_LEGACY_PUSH_LOCKS)
#define KTL_EX_PUSH_LOCKS
#endif

#include <mutex.hpp>

namespace ktl {
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
  KeEnterCriticalRegion();
  ExAcquirePushLockExclusive(native_handle());
}

void push_lock::lock_shared() {
  KeEnterCriticalRegion();
  ExAcquirePushLockShared(native_handle());
}

void push_lock::unlock() {
  ExReleasePushLockExclusive(native_handle());
  KeLeaveCriticalRegion();
}

void push_lock::unlock_shared() {
  ExReleasePushLockShared(native_handle());
  KeLeaveCriticalRegion();
}
#endif
}