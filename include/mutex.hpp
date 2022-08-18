#pragma once
#include <basic_types.hpp>
#include <chrono.hpp>
#include <compressed_pair.hpp>
#include <irql.hpp>
#include <thread.hpp>
#include <tuple.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

#include <ntddk.h>

namespace ktl {  // threads management
namespace th::details {
template <class SyncPrimitive>
class sync_primitive_base : non_relocatable {
 public:
  using sync_primitive_t = SyncPrimitive;
  using native_handle_type = sync_primitive_t*;

 protected:
  sync_primitive_base() = default;

  [[nodiscard]] native_handle_type get_non_const_native_handle()
      const noexcept {
    return const_cast<native_handle_type>(addressof(m_native_sp));
  }

 public:
  native_handle_type native_handle() noexcept { return addressof(m_native_sp); }

 protected:
  SyncPrimitive m_native_sp;
};
}  // namespace th::details

class recursive_mutex
    : public th::details::sync_primitive_base<KMUTEX> {  // Recursive KMUTEX
 public:
  using MyBase = sync_primitive_base<KMUTEX>;

 public:
  recursive_mutex() noexcept;

  void lock() noexcept;
  void unlock() noexcept;

  template <class Awaitable, class AwaitHandler>
  void unlock(Awaitable& awaitable, AwaitHandler await_handler) {
    const auto state{KeReleaseMutex(native_handle(), true)};
    await_handler(awaitable, state != 0);
  }

 protected:
  static void wait_impl(KMUTEX* mtx, const LARGE_INTEGER* timeout);
};

using mutex = recursive_mutex;

struct timed_recursive_mutex : recursive_mutex {  // Locking with timeout
  using MyBase = recursive_mutex;
  using MyBase::MyBase;

  template <class Rep, class Period>
  void lock_for(const chrono::duration<Rep, Period>& wait_duration) {
    using namespace th::details;
    if (const auto await_status = wait_for_impl<ZeroWaitPolicy::Cancel>(
            wait_duration,
            [mtx = native_handle()](LARGE_INTEGER* interval) {
              wait_impl(mtx, interval);
            });
        await_status == STATUS_CANCELLED) {
      lock();
    }
  }

  template <class Clock, class Duration>
  void lock_until(const chrono::time_point<Clock, Duration>& awake_time) {
    using namespace th::details;
    if (const auto await_status = wait_until_impl<ZeroWaitPolicy::Cancel>(
            awake_time,
            [mtx = native_handle()](LARGE_INTEGER* interval) {
              wait_impl(mtx, interval);
            });
        await_status == STATUS_CANCELLED) {
      lock();
    }
  }
};

using timed_mutex = timed_recursive_mutex;

struct fast_mutex
    : th::details::sync_primitive_base<FAST_MUTEX> {  // Implemented
                                                      // using
                                                      // non-recursive
                                                      // FAST_MUTEX
  using MyBase = sync_primitive_base<FAST_MUTEX>;

  fast_mutex() noexcept;

  void lock() noexcept;
  bool try_lock() noexcept;
  void unlock() noexcept;
};

struct shared_mutex
    : th::details::sync_primitive_base<ERESOURCE> {  // Wrapper for
                                                     // ERESOURCE
  using MyBase = sync_primitive_base<ERESOURCE>;

  shared_mutex();
  ~shared_mutex() noexcept;

  void lock() noexcept;
  void lock_shared() noexcept;
  void unlock() noexcept;
  void unlock_shared() noexcept;
};

struct push_lock : ktl::th::details::sync_primitive_base<EX_PUSH_LOCK> {
  using MyBase = ktl::th::details::sync_primitive_base<EX_PUSH_LOCK>;

  push_lock() noexcept;
  ~push_lock() noexcept;

  void lock();
  void lock_shared();
  void unlock();
  void unlock_shared();
};

namespace th::details {
template <class LockPolicy>
class spin_lock_base : non_relocatable {
 public:
  using sync_primitive_t = KSPIN_LOCK;
  using native_handle_type = sync_primitive_t*;

 protected:
  using policy_type = LockPolicy;

 private:
  static_assert(is_nothrow_default_constructible_v<LockPolicy>,
                "LockPolicy must by nothrow default constructible");

 public:
  spin_lock_base() noexcept { KeInitializeSpinLock(native_handle()); }

  native_handle_type native_handle() noexcept {
    return addressof(m_storage.get_second());
  }

 protected:
  policy_type& get_policy() noexcept { return m_storage.get_first(); }

 private:
  compressed_pair<LockPolicy, KSPIN_LOCK> m_storage;
};

enum class SpinlockType { Mixed, DpcOnly };

template <SpinlockType Type>
class spin_lock_policy_base {
 protected:
  mutable irql_t m_prev_irql{};
};

template <>
class spin_lock_policy_base<SpinlockType::DpcOnly> {};

template <SpinlockType Type>
struct spin_lock_policy : spin_lock_policy_base<Type> {
  void lock(KSPIN_LOCK& target) const noexcept;
  bool try_lock(KSPIN_LOCK& target) const noexcept;
  void unlock(KSPIN_LOCK& target) const noexcept;
};

template <SpinlockType Type>
struct queued_spin_lock_policy {
  void lock(KSPIN_LOCK& target,
            KLOCK_QUEUE_HANDLE& queue_handle) const noexcept;
  void unlock(KLOCK_QUEUE_HANDLE& queue_handle) const noexcept;
};

inline constexpr uint32_t INTERRUPT_SPIN_LOCK{
    0x0},                     // Acquired and released at DIRQL
    NORMAL_SPIN_LOCK{0x1},    // Acquired and released at IRQL <= DISPATCH_EVEL
    DPC_ONLY_SPIN_LOCK{0x2},  // Acquired and released at DISPATCH_EVEL
    LOW_IRQL_SPIN_LOCK{0x3};  // Acquired and released at IRQL < DISPATCH_EVEL

template <uint8_t>
struct spin_lock_selector_impl;

template <>
struct spin_lock_selector_impl<INTERRUPT_SPIN_LOCK>;  // Unsupported now

template <>
struct spin_lock_selector_impl<NORMAL_SPIN_LOCK> {
  static constexpr SpinlockType value = SpinlockType::Mixed;
};

template <>
struct spin_lock_selector_impl<DPC_ONLY_SPIN_LOCK> {
  static constexpr SpinlockType value = SpinlockType::DpcOnly;
};

template <>
struct spin_lock_selector_impl<LOW_IRQL_SPIN_LOCK> {
  static constexpr SpinlockType value = SpinlockType::Mixed;
};

template <irql_t MinIrql, irql_t MaxIrql>
struct spin_lock_type_selector {
  static_assert(MinIrql <= MaxIrql,
                "Invalid spinlock type: MinIrql must be less or equal MaxIrql");
  static_assert(MaxIrql <= DISPATCH_LEVEL,
                "Interrupt spinlocks are unsupported now");

  static constexpr SpinlockType value =
      spin_lock_selector_impl<static_cast<uint8_t>(MinIrql <= APC_LEVEL) |
                              (static_cast<uint8_t>(MaxIrql <= DISPATCH_LEVEL)
                               << 1)>::value;
};

template <irql_t MinIrql, irql_t MaxIrql>
inline constexpr SpinlockType spin_lock_type_v =
    spin_lock_type_selector<MinIrql, MaxIrql>::value;

template <irql_t MinIrql, irql_t MaxIrql>
using spin_lock_policy_selector_t =
    spin_lock_policy<spin_lock_type_v<MinIrql, MaxIrql>>;

template <irql_t MinIrql, irql_t MaxIrql>
using queued_spin_lock_policy_selector_t =
    queued_spin_lock_policy<spin_lock_type_v<MinIrql, MaxIrql>>;
}  // namespace th::details

template <irql_t MinIrql = PASSIVE_LEVEL, irql_t MaxIrql = DISPATCH_LEVEL>
struct spin_lock
    : th::details::spin_lock_base<
          th::details::spin_lock_policy_selector_t<MinIrql, MaxIrql>> {
  using MyBase = th::details::spin_lock_base<
      th::details::spin_lock_policy_selector_t<MinIrql, MaxIrql>>;

  using MyBase::MyBase;

  void lock() noexcept { MyBase::get_policy().lock(*native_handle()); }
  bool try_lock() noexcept {
    return MyBase::get_policy().try_lock(*native_handle());
  }
  void unlock() noexcept { MyBase::get_policy().unlock(*native_handle()); }

  using MyBase::native_handle;
};

template <irql_t MinIrql = PASSIVE_LEVEL, irql_t MaxIrql = DISPATCH_LEVEL>
struct queued_spin_lock
    : th::details::spin_lock_base<
          th::details::queued_spin_lock_policy_selector_t<MinIrql, MaxIrql>> {
  using MyBase = th::details::spin_lock_base<
      th::details::queued_spin_lock_policy_selector_t<MinIrql, MaxIrql>>;
  using owner_handle_type = KLOCK_QUEUE_HANDLE;

  using MyBase::MyBase;

  void lock(owner_handle_type& owner_handle) noexcept {
    MyBase::get_policy().lock(*native_handle(), owner_handle);
  }
  void unlock(owner_handle_type& owner_handle) noexcept {
    MyBase::get_policy().unlock(owner_handle);
  }

  using MyBase::native_handle;
};

struct semaphore : th::details::sync_primitive_base<KSEMAPHORE> {
  using MyBase = sync_primitive_base<KSEMAPHORE>;
  using counter_type = long;

  semaphore(counter_type start_count,
            counter_type upper_limit =
                (numeric_limits<counter_type>::max)()) noexcept;

  void acquire() noexcept;
  void release() noexcept;
};

enum class cv_status : uint8_t { no_timeout, timeout };
using event_type = EVENT_TYPE;

namespace th::details {
struct event_base : sync_primitive_base<KEVENT> {
  using MyBase = sync_primitive_base<KEVENT>;

  event_base(event_type type, bool signaled = false) noexcept;
  event_base& operator=(bool signaled) noexcept;

  void clear() noexcept;
  bool reset() noexcept;

  bool set(KPRIORITY priority_boost = 0) noexcept;

  template <class Awaitable, class AwaitHandler>
  void set(Awaitable& awaitable,
           AwaitHandler await_handler,
           KPRIORITY priority_boost = 0) noexcept {
    const auto state{KeSetEvent(native_handle(), priority_boost, true)};
    await_handler(awaitable, state != 0);
  }

  bool pulse(KPRIORITY priority_boost = 0) noexcept;

  template <class Awaitable, class AwaitHandler>
  void pulse(Awaitable& awaitable,
             AwaitHandler handler,
             KPRIORITY priority_boost = 0) noexcept {
    const auto state{KePulseEvent(native_handle(), priority_boost, true)};
    await_handler(awaitable, state != 0);
  }

  void wait() noexcept;

  template <class Predicate>
  void wait(Predicate predicate) {
    while (!predicate()) {
      wait();
    }
  }

  template <class Rep, class Period>
  cv_status wait_for(
      const chrono::duration<Rep, Period>& wait_duration) noexcept {
    // Behavior closer to std::condition_variable instead of typical
    // KeWaitForSingleObject(event, ..., &zero_timeout)
    const auto await_status{wait_for_impl<ZeroWaitPolicy::Cancel>(
        wait_duration, [event = native_handle()](LARGE_INTEGER* interval) {
          return wait_impl(event, interval);
        })};
    return from_native_status(await_status);
  }

  template <class Rep, class Period, class Predicate>
  bool wait_for(const chrono::duration<Rep, Period>& wait_duration,
                Predicate predicate) noexcept {
    return wait_until(chrono::steady_clock::now() + wait_duration,
                      move(predicate));
  }

  template <class Clock, class Duration, class Predicate>
  bool wait_until(const chrono::time_point<Clock, Duration>& awake_time,
                  Predicate predicate) noexcept {
    while (!predicate()) {
      if (wait_until(awake_time) == cv_status::timeout) {
        return predicate();
      }
    }
    return true;
  }

  template <class Clock, class Duration>
  cv_status wait_until(
      const chrono::time_point<Clock, Duration>& awake_time) noexcept {
    // Behavior closer to std::condition_variable instead of typical
    // KeWaitForSingleObject(event, ..., &zero_timeout)
    const auto await_status{wait_until_impl<ZeroWaitPolicy::Cancel>(
        awake_time, [event = native_handle()](LARGE_INTEGER* interval) {
          return wait_impl(event, interval);
        })};
    return from_native_status(await_status);
  }

  [[nodiscard]] bool is_signaled() const noexcept;
  explicit operator bool() const noexcept;

 private:
  template <class AwaitToken, class Handler>
  static void bump_impl(native_handle_type event,
                        AwaitToken token,
                        Handler handler) noexcept {
    if (!token.is_awaitable()) {
      handler(event, token.get_priority_boost(), false);
    } else {
      handler(event, token.get_priority_boost(), true);
      wait_impl(event, token.get_timeout());
    }
  }

  static cv_status from_native_status(NTSTATUS status) noexcept;

  static NTSTATUS wait_impl(native_handle_type event,
                            const LARGE_INTEGER* timeout) noexcept;
};

template <event_type Type>
struct event_with_fixed_type : event_base {
  using MyBase = event_base;

  event_with_fixed_type(bool signaled = false) noexcept
      : MyBase(Type, signaled) {}
};
}  // namespace th::details

struct sync_event : th::details::event_with_fixed_type<SynchronizationEvent> {
  using MyBase = event_with_fixed_type<SynchronizationEvent>;
  using MyBase::MyBase;
};

struct notify_event : th::details::event_with_fixed_type<NotificationEvent> {
  using MyBase = event_with_fixed_type<NotificationEvent>;
  using MyBase::MyBase;
};

namespace th::details {
template <class Lockable, class Rep, class Period, class = void>
struct has_lock_for : false_type {};

template <class Lockable, class Rep, class Period>
struct has_lock_for<Lockable,
                    Rep,
                    Period,
                    void_t<decltype(declval<Lockable>().lock_for(
                        declval<chrono::duration<Rep, Period>>()))>>
    : true_type {};

template <class Lockable, class Rep, class Period>
inline constexpr bool has_lock_for_v =
    has_lock_for<Lockable, Rep, Period>::value;

template <class Lockable, class Clock, class Duration, class = void>
struct has_lock_until : false_type {};

template <class Lockable, class Clock, class Duration>
struct has_lock_until<Lockable,
                      Clock,
                      Duration,
                      void_t<decltype(declval<Lockable>().lock_until(
                          declval<chrono::time_point<Clock, Duration>>()))>>
    : true_type {};

template <class Lockable, class Clock, class Duration>
inline constexpr bool has_lock_until_v =
    has_lock_until<Lockable, Clock, Duration>::value;
}  // namespace th::details

struct defer_lock_tag {};    // Deferred lock
struct try_lock_for_tag {};  // Lock with timeout
struct adopt_lock_tag {};    // Mutex is already held

namespace th::details {
template <class Mutex>
class mutex_guard_storage {
 public:
  using mutex_type = Mutex;

 public:
  constexpr mutex_guard_storage(mutex_type& mtx) noexcept
      : m_mtx{addressof(mtx)} {}

 protected:
  mutex_type* mutex() const noexcept { return m_mtx; }

 protected:
  mutex_type* m_mtx{nullptr};
};

template <class Mutex>
class mutex_guard_base : non_copyable {
 public:
  using mutex_type = Mutex;

 public:
  void swap(mutex_guard_base& other) noexcept {
    swap(m_mtx, other.m_mtx);
    swap(m_owned, other.m_owned);
  }

  [[nodiscard]] bool owns_lock() const noexcept { return m_owned; }
  Mutex* mutex() noexcept { return m_mtx; }

 protected:
  constexpr mutex_guard_base() noexcept = default;
  mutex_guard_base(Mutex& mtx, bool owned = false) noexcept
      : m_mtx{addressof(mtx)}, m_owned{owned} {}

  mutex_guard_base& move_construct_from(mutex_guard_base&& other) noexcept {
    m_mtx = exchange(other.m_mtx, nullptr);
    m_owned = exchange(other.m_owned, false);
    return *this;
  }

  void lock() {
    m_mtx->lock();
    m_owned = true;
  }

  void lock_shared() {
    m_mtx->lock_shared();
    m_owned = true;
  }

  template <class Rep, class Period>
  void lock_for(const chrono::duration<Rep, Period>& wait_duration) {
    static_assert(has_lock_for_v<mutex_type, Rep, Period>,
                  "Mutex doesn't support locking with timeout");
    m_mtx->lock_for(wait_duration);
    m_owned = true;
  }

  template <class Clock, class Duration>
  void lock_until(const chrono::time_point<Clock, Duration>& awake_time) {
    static_assert(has_lock_until_v<mutex_type, Clock, Duration>,
                  "Mutex doesn't support locking with timeout");
    m_mtx->lock_until(awake_time);
    m_owned = true;
  }

  void unlock() {
    m_mtx->unlock();
    m_owned = false;
  }

  void unlock_shared() {
    m_mtx->unlock_shared();
    m_owned = false;
  }

  mutex_type* release() {
    m_owned = false;
    return exchange(m_mtx, nullptr);
  }

 protected:
  mutex_type* m_mtx{nullptr};
  bool m_owned{false};
};

template <class Mutex, class = void>
class lock_guard_base : public mutex_guard_storage<Mutex>, non_relocatable {
 public:
  using MyBase = mutex_guard_storage<Mutex>;
  using mutex_type = typename MyBase::mutex_type;

 public:
  lock_guard_base(mutex_type& mtx) : MyBase(mtx) { MyBase::mutex()->lock(); }
  lock_guard_base(mutex_type mtx, adopt_lock_tag) : MyBase(mtx) {}
  ~lock_guard_base() { MyBase::mutex()->unlock(); }
};

template <class Mutex>
class lock_guard_base<Mutex, void_t<typename Mutex::owner_handle_type>>
    : public mutex_guard_storage<Mutex>, non_relocatable {
 public:
  using MyBase = mutex_guard_storage<Mutex>;
  using mutex_type = typename MyBase::mutex_type;
  using owner_handle_type = typename mutex_type::owner_handle_type;

 public:
  lock_guard_base(mutex_type& mtx) : MyBase(mtx) {
    MyBase::mutex()->lock(m_owner_handle);
  }
  lock_guard_base(mutex_type mtx, adopt_lock_tag) : MyBase(mtx) {}
  ~lock_guard_base() { MyBase::mutex()->unlock(m_owner_handle); }

 private:
  owner_handle_type m_owner_handle{};
};
}  // namespace th::details

template <class Mutex>
struct lock_guard : th::details::lock_guard_base<Mutex> {
  using MyBase = th::details::lock_guard_base<Mutex>;
  using MyBase::MyBase;
};

template <class Mutex>
lock_guard(Mutex&)
    -> lock_guard<typename th::details::mutex_guard_base<Mutex>::mutex_type>;

template <class Mutex>
lock_guard(Mutex&, adopt_lock_tag)
    -> lock_guard<typename th::details::mutex_guard_base<Mutex>::mutex_type>;

template <class Mutex>
class unique_lock : public th::details::mutex_guard_base<Mutex> {
 public:
  using MyBase = th::details::mutex_guard_base<Mutex>;
  using mutex_type = typename MyBase::mutex_type;

 public:
  constexpr unique_lock() = default;
  explicit unique_lock(mutex_type& mtx) : MyBase(mtx) { MyBase::lock(); }
  unique_lock(mutex_type& mtx, adopt_lock_tag) : MyBase(mtx, true) {}
  unique_lock(mutex_type& mtx, defer_lock_tag) : MyBase(mtx) {}

  template <class Mtx = mutex_type,
            class Rep,
            class Period,
            enable_if_t<th::details::has_lock_for_v<Mtx, Rep, Period>, int> = 0>
  unique_lock(mutex_type& mtx,
              const chrono::duration<Rep, Period>& wait_duration,
              try_lock_for_tag)
      : MyBase(mtx) {
    MyBase::lock_for(wait_duration);
  }

  unique_lock(const unique_lock& other) = delete;
  unique_lock operator=(const unique_lock& other) = delete;

  unique_lock(unique_lock&& other) noexcept {
    MyBase::move_construct_from(move(other));
  }

  unique_lock& operator=(unique_lock&& other) noexcept {
    reset_if_needed();
    MyBase::move_construct_from(move(other));
    return *this;
  }

  ~unique_lock() { reset_if_needed(); }

  operator bool() const noexcept { return MyBase::owns(); }

  using MyBase::lock;
  using MyBase::release;
  using MyBase::unlock;

 private:
  void reset_if_needed() noexcept {
    if (MyBase::owns_lock()) {
      MyBase::unlock();
    }
  }
};

template <class Mutex>
unique_lock(Mutex&)
    -> unique_lock<typename th::details::mutex_guard_base<Mutex>::mutex_type>;

template <class Mutex>
unique_lock(Mutex&, adopt_lock_tag)
    -> unique_lock<typename th::details::mutex_guard_base<Mutex>::mutex_type>;

template <class Mutex>
unique_lock(Mutex&, defer_lock_tag)
    -> unique_lock<typename th::details::mutex_guard_base<Mutex>::mutex_type>;

template <class Mutex>
unique_lock(Mutex&,
            typename th::details::mutex_guard_base<Mutex>::duration_t,
            try_lock_for_tag)
    -> unique_lock<typename th::details::mutex_guard_base<Mutex>::mutex_type>;

template <class Mutex>
class shared_lock : public th::details::mutex_guard_base<Mutex> {
 public:
  using MyBase = th::details::mutex_guard_base<Mutex>;
  using mutex_type = typename MyBase::mutex_type;

 public:
  constexpr shared_lock() = default;
  explicit shared_lock(mutex_type& mtx) : MyBase(mtx) { MyBase::lock_shared(); }
  shared_lock(mutex_type& mtx, adopt_lock_tag) : MyBase(mtx, true) {}
  shared_lock(mutex_type& mtx, defer_lock_tag) : MyBase(mtx) {}

  template <class Mtx = mutex_type,
            class Rep,
            class Period,
            enable_if_t<th::details::has_lock_for_v<Mtx, Rep, Period>, int> = 0>
  shared_lock(mutex_type& mtx,
              const chrono::duration<Rep, Period>& wait_duration,
              try_lock_for_tag)
      : MyBase(mtx) {
    MyBase::lock_for(wait_duration);
  }

  shared_lock(const shared_lock& other) = delete;
  shared_lock operator=(const shared_lock& other) = delete;

  shared_lock(shared_lock&& other) noexcept {
    MyBase::move_construct_from(move(other));
  }

  shared_lock& operator=(shared_lock&& other) noexcept {
    reset_if_needed();
    MyBase::move_construct_from(move(other));
    return *this;
  }

  ~shared_lock() { reset_if_needed(); }

  operator bool() const noexcept { return MyBase::owns_lock(); }

  using MyBase::release;

  void lock() { MyBase::lock_shared; }
  void unlock() { MyBase::unlock_shared; }

 private:
  void reset_if_needed() noexcept {
    if (MyBase::owns_lock()) {
      MyBase::unlock_shared();
    }
  }
};

template <class Mutex>
shared_lock(Mutex&)
    -> shared_lock<typename th::details::mutex_guard_base<Mutex>::mutex_type>;

template <class Mutex>
shared_lock(Mutex&, adopt_lock_tag)
    -> shared_lock<typename th::details::mutex_guard_base<Mutex>::mutex_type>;

template <class Mutex>
shared_lock(Mutex&, defer_lock_tag)
    -> shared_lock<typename th::details::mutex_guard_base<Mutex>::mutex_type>;

template <class Mutex>
shared_lock(Mutex&,
            typename th::details::mutex_guard_base<Mutex>::duration_t,
            try_lock_for_tag)
    -> shared_lock<typename th::details::mutex_guard_base<Mutex>::mutex_type>;

class irql_guard {
 public:
  using irql_t = KIRQL;

 public:
  irql_guard(irql_t new_irql) : m_old_irql{raise_irql(new_irql)} {}
  ~irql_guard() { lower_irql(m_old_irql); }

 private:
  static irql_t raise_irql(irql_t new_irql) {
    irql_t old_irql;
    KeRaiseIrql(new_irql, addressof(old_irql));
    return old_irql;
  }

  static void lower_irql(irql_t new_irql) { return KeLowerIrql(new_irql); }

 private:
  irql_t m_old_irql;
};

namespace th::details {

template <class... Mtxs, size_t... Idxs>
void lock_target_from_locks(const int target,
                            index_sequence<Idxs...>,
                            Mtxs&... mtxs) {
  [[maybe_unused]] int ignored[] = {
      ((target == static_cast<int>(Idxs)) ? (((void)mtxs.lock()), 0) : 0)...};
}

template <class... Mtxs, size_t... Idxs>
bool try_lock_target_from_locks(const int target,
                                index_sequence<Idxs...>,
                                Mtxs&... mtxs) {
  bool result = false;

  [[maybe_unused]] int ignored[] = {((target == static_cast<int>(Idxs))
                                         ? ((result = mtxs.try_lock()), 0)
                                         : 0)...};

  return result;
}

template <class... Mtxs, size_t... Idxs>
void unlock_locks(const int first,
                  const int last,
                  index_sequence<Idxs...>,
                  Mtxs&... mtxs) {
  [[maybe_unused]] int ignored[] = {
      ((first <= static_cast<int>(Idxs) && static_cast<int>(Idxs) < last)
           ? (((void)mtxs.unlock()), 0)
           : 0)...};
}

template <class... Mtxs>
int try_lock_range(const int first, const int last, Mtxs&... mtxs) {
  using indices_t = index_sequence_for<Mtxs...>;

  int current = first;

  try {
    for (; current < last; ++current) {
      if (!try_lock_target_from_locks(current, indices_t{}, mtxs...)) {
        unlock_locks(first, current, indices_t{}, mtxs...);
        return current;
      }
    }
  } catch (...) {
    unlock_locks(first, current, indices_t{}, mtxs...);
    throw;
  }

  return -1;
}

template <class... Mtxs>
int lock_attempt(const int prev, Mtxs&... mtxs) {
  using indices_t = index_sequence_for<Mtxs...>;
  lock_target_from_locks(prev, indices_t{}, mtxs...);

  int failed = -1;
  int rollback_start = prev;

  try {
    failed = try_lock_range(0, prev, mtxs...);
    if (failed == -1) {
      rollback_start = 0;
      failed = try_lock_range(prev + 1, sizeof...(Mtxs), mtxs...);
      if (failed == -1) {
        return -1;
      }
    }
  } catch (...) {
    unlock_locks(rollback_start, prev + 1, indices_t{}, mtxs...);
    throw;
  }

  unlock_locks(rollback_start, prev + 1, indices_t{}, mtxs...);
  return failed;
}

template <class Mtx1, class Mtx2, class Mtx3, class... MtxNs>
void lock_impl(Mtx1& mtx1, Mtx2& mtx2, Mtx3& mtx3, MtxNs&... mtxns) {
  int not_locked = 0;
  while (not_locked != -1) {
    not_locked = lock_attempt(not_locked, mtx1, mtx2, mtx3, mtxns...);
  }
}

template <class Mtx1, class Mtx2>
bool lock_attempt_small(Mtx1& mtx1, Mtx2& mtx2) {
  mtx1.lock();

  try {
    if (mtx2.try_lock()) {
      return false;
    }
  } catch (...) {
    mtx1.unlock();
    throw;
  }

  mtx1.unlock();
  this_thread::yield();

  return true;
}

template <class Mtx1, class Mtx2>
void lock_impl(Mtx1& mtx1, Mtx2& mtx2) {
  while (lock_attempt_small(mtx1, mtx2) && lock_attempt_small(mtx2, mtx1))
    /* Don't give up... Try harder! */;
}

}  // namespace th::details

template <class Mtx1, class Mtx2, class... MtxNs>
void lock(Mtx1& mtx1, Mtx2& mtx2, MtxNs&... mtxns) {
  th::details::lock_impl(mtx1, mtx2, mtxns...);
}

template <class... Mtxs>
class scoped_lock : non_copyable {
 public:
  explicit scoped_lock(Mtxs&... mtxs) : m_mtxs(mtxs...) { ktl::lock(mtxs...); }

  explicit scoped_lock(adopt_lock_tag, Mtxs&... mtxs)
      : m_mtxs(mtxs...) { /* Don't lock */
  }

  ~scoped_lock() {
    apply([](Mtxs&... mtxs) { (..., (void)mtxs.unlock()); }, m_mtxs);
  }

 private:
  tuple<Mtxs&...> m_mtxs;
};

template <class Mtx>
class scoped_lock<Mtx> : non_copyable {
 public:
  using mutex_type = Mtx;

  explicit scoped_lock(Mtx& mtx) : m_mtx(mtx) { m_mtx.lock(); }

  explicit scoped_lock(adopt_lock_tag, Mtx& mtx) : m_mtx(mtx) { /* Don't lock */
  }

  ~scoped_lock() { m_mtx.unlock(); }

 private:
  Mtx& m_mtx;
};

template <>
class scoped_lock<> : non_copyable {
 public:
  explicit scoped_lock() = default;
  explicit scoped_lock(adopt_lock_tag) {}
};

template <class... Mtxs>
scoped_lock(Mtxs&...) -> scoped_lock<Mtxs...>;

template <class... Mtxs>
scoped_lock(adopt_lock_tag, Mtxs&...) -> scoped_lock<Mtxs...>;
}  // namespace ktl
