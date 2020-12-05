#pragma once
#include <basic_types.h>
#include <type_traits.hpp>
#include <utility.hpp>
#include <chrono.hpp>
#include <atomic.hpp>

#include <ntddk.h>

namespace ktl {  // threads management
namespace th::details {
template <class SyncPrimitive>
class sync_primitive_base {
 public:
  using sync_primitive_t = SyncPrimitive;
  using native_handle_t = sync_primitive_t*;

 public:
  template <class SP>
  sync_primitive_base(const sync_primitive_base<SP>&) = delete;
  template <class SP>
  sync_primitive_base& operator=(const sync_primitive_base<SP>&) = delete;

  native_handle_t native_handle() noexcept { return addressof(m_native_sp); }

 protected:
  sync_primitive_base() = default;

 protected:
  SyncPrimitive m_native_sp;
};

template <class SyncPrimitive>
class waitable_sync_primitive
    : public sync_primitive_base<SyncPrimitive> {  //Примитивы синхронизации,
                                                   //поддерживающие ожидание с
                                                   //таймером
 public:
  using MyBase = th::details::sync_primitive_base<KMUTEX>;
  using sync_primitive_t = typename MyBase::sync_primitive_t;
  using native_handle_t = typename MyBase::native_handle_t;
  using duration_t = chrono::duration_t;

 protected:
  bool try_timed_lock(duration_t timeout_duration) {
    LARGE_INTEGER
    period{chrono::to_native_100ns_tics(timeout_duration)};
    auto status{KeWaitForSingleObject(MyBase::native_handle(),
                                      Executive,   // Wait reason
                                      KernelMode,  // Processor mode
                                      false, addressof(period))};
    return NT_SUCCESS(status);
  }
  void lock_indefinite() {
    KeWaitForSingleObject(MyBase::native_handle(),
                          Executive,      // Wait reason
                          KernelMode,     // Processor mode
                          false, nullptr  // Indefinite waiting
    );
  }
};
}  // namespace details

class recursive_mutex
    : public th::details::waitable_sync_primitive<KMUTEX> {  // Recursive KMUTEX
 public:
  using MyBase = th::details::waitable_sync_primitive<KMUTEX>;
  using sync_primitive_t = typename MyBase::sync_primitive_t;
  using native_handle_t = typename MyBase::native_handle_t;

 public:
  recursive_mutex() { KeInitializeMutex(native_handle(), 0); }

  void lock() { MyBase::lock_indefinite(); }

  void unlock() { KeReleaseMutex(MyBase::native_handle(), false); }
};

class timed_recursive_mutex : public recursive_mutex {  //Блокировка с таймером
 public:
  using MyBase = recursive_mutex;
  using sync_primitive_t = typename MyBase::sync_primitive_t;
  using native_handle_t = typename MyBase::native_handle_t;
  using duration_t = typename MyBase::duration_t;

 public:
  using MyBase::MyBase;

  bool try_lock_for(duration_t timeout_duration) {
    return MyBase::try_timed_lock(timeout_duration);
  }
};

class fast_mutex
    : public th::details::sync_primitive_base<FAST_MUTEX> {  // Implemented using
                                                         // non-recursive
                                                         // FAST_MUTEX
 public:
  using MyBase = th::details::sync_primitive_base<FAST_MUTEX>;
  using sync_primitive_t = typename MyBase::sync_primitive_t;
  using native_handle_t = typename MyBase::native_handle_t;

 public:
  fast_mutex() { ExInitializeFastMutex(MyBase::native_handle()); }

  void lock() { ExAcquireFastMutex(MyBase::native_handle()); }
  bool try_lock() { return ExTryToAcquireFastMutex(MyBase::native_handle()); }
  void unlock() { ExReleaseFastMutex(MyBase::native_handle()); }
};

class shared_mutex
    : public th::details::sync_primitive_base<ERESOURCE> {  // Wrapper for
                                                        // ERESOURCE

 public:
  using MyBase = th::details::sync_primitive_base<ERESOURCE>;
  using sync_primitive_t = typename MyBase::sync_primitive_t;
  using native_handle_t = typename MyBase::native_handle_t;

 public:
  shared_mutex() { ExInitializeResourceLite(native_handle()); }

  void lock() {  //Вход в критическую секцию
    ExEnterCriticalRegionAndAcquireResourceExclusive(native_handle());
  }
  void lock_shared() {
    ExEnterCriticalRegionAndAcquireResourceShared(native_handle());
  }
  void unlock() {  //Выход из критической секции
    ExReleaseResourceAndLeaveCriticalRegion(native_handle());
  }
  void unlock_shared() {
    ExReleaseResourceAndLeaveCriticalRegion(native_handle());
  }
};

class dpc_spin_lock
    : public th::details::sync_primitive_base<KSPIN_LOCK> {  // DISPATCH_LEVEL spin
                                                         // lock
 public:
  using MyBase = th::details::sync_primitive_base<KSPIN_LOCK>;
  using sync_primitive_t = typename MyBase::sync_primitive_t;
  using native_handle_t = typename MyBase::native_handle_t;

 public:
  dpc_spin_lock() { KeInitializeSpinLock(native_handle()); }

  void lock() { KeAcquireSpinLockAtDpcLevel(native_handle()); }
  void unlock() { KeReleaseSpinLockFromDpcLevel(native_handle()); }
};

class semaphore
    : public th::details::sync_primitive_base<KSEMAPHORE> {  // Implemented using
                                                         // KSEMAPHORE
 public:
  using MyBase = th::details::sync_primitive_base<KSEMAPHORE>;
  using sync_primitive_t = typename MyBase::sync_primitive_t;
  using native_handle_t = typename MyBase::native_handle_t;

 public:
  struct Settings {
    uint32_t start_count{1};
    uint32_t upper_limit{
        static_cast<uint32_t>(-1)};  // TODO: ввести класс numeric_limits
  };

 public:
  semaphore(Settings settings) {
    KeInitializeSemaphore(native_handle(),
                          static_cast<LONG>(settings.start_count),
                          static_cast<LONG>(settings.upper_limit));
  }

  void acquire() {
    KeWaitForSingleObject(native_handle(),
                          Executive,   // Wait reason
                          KernelMode,  // Processor mode
                          false,
                          nullptr  // Indefinite waiting
    );
  }
  void release() {
    KeReleaseSemaphore(native_handle(), HIGH_PRIORITY, 1, false);
  }
};

class event : public th::details::sync_primitive_base<KEVENT> {  // Implemented
                                                             // using KSEMAPHORE
 public:
  using MyBase = th::details::sync_primitive_base<KEVENT>;
  using sync_primitive_t = typename MyBase::sync_primitive_t;
  using native_handle_t = typename MyBase::native_handle_t;

 public:
  enum class State { Active, Inactive };

 public:
  event(State state = State::Inactive) {
    bool activated{state == State::Active};
    KeInitializeEvent(native_handle(), SynchronizationEvent, activated);
    update_state(state);
  }
  event& operator=(State state) { update_state(state); }

  void wait_for(size_t us) {
    LARGE_INTEGER wait_time;
    wait_time.QuadPart = us;
    KeWaitForSingleObject(native_handle(),
                          Executive,   // Wait reason
                          KernelMode,  // Processor mode
                          false,
                          addressof(wait_time)  // Indefinite waiting
    );
  }
  void set() { update_state(State::Active); }
  void clear() { update_state(State::Inactive); }
  State signaled() const noexcept { return m_state; }

 private:
  void update_state(State new_state) {
    if (new_state != m_state) {
      switch (m_state) {
        case State::Active:
          KeSetEvent(native_handle(), HIGH_PRIORITY, false);
          break;
        case State::Inactive:
          KeClearEvent(native_handle());
          break;
        default:
          break;
      };
      m_state = new_state;
    }
  }

 private:
  State m_state;
};

namespace th::details {
bool unlock_impl() {
  return true;  // Dummy
}

template <class FirstLocable, class... RestLockables>
bool unlock_impl(FirstLocable& first, RestLockables&... rest) {
  bool current_unlock_success{true};
  __try {
    first.unlock();
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    current_unlock_success = false;
  }
  return current_unlock_success && unlock_impl(rest...);
}
}  // namespace details

template <class... Lockable>
void unlock(Lockable&... locables) {
  th::details::unlock_impl(locables);
}

namespace th::details {
bool lock_impl() {
  return true;  // Dummy
}

template <class FirstLocable, class... RestLockables>
bool lock_impl(FirstLocable& first, RestLockables&... rest) {
  __try {
    first.lock();
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
  bool lock_chain_result{lock_impl(rest...)};
  if (!lock_chain_result) {
    unlock(first);
  }
  return lock_chain_result;
}
}  // namespace details

template <class... Lockable>
void lock(Lockable&... locables) {
  th::details::lock_impl(locables);
}

namespace th::details {
template <class Locable, class Duration, class = void>
struct has_try_lock_for : false_type {};

template <class Locable, class Duration>
struct has_try_lock_for<
    Locable,
    Duration,
    void_t<decltype(declval<Locable>().try_lock_for(declval<Duration>()))>>
    : true_type {};

template <class Locable, class Duration>
inline constexpr bool has_try_lock_for_v = false;
// has_try_lock_for<Locable, Duration>::value;

}  // namespace details

struct defer_lock_tag {};    //Отложенная блокировка
struct try_lock_for_tag {};  //Блокировка с таймаутом
struct adopt_lock_tag {};  //Сигнализирует, что мьютекс уже захвачен

namespace th::details {
template <class Mutex, class = void>
struct get_duration_type {
  using type = chrono::duration_t;
};

template <class Mutex>
struct get_duration_type<Mutex, void_t<typename Mutex::duration_t>> {
  using type = typename Mutex::duration_t;
};

template <class Mutex>
using get_duration_type_t = typename get_duration_type<Mutex>::type;

template <class Mutex>
class mutex_guard_base {
 public:
  using mutex_t = Mutex;
  using duration_t = get_duration_type_t<mutex_t>;

 public:
  mutex_guard_base(const mutex_guard_base& mtx) = delete;
  mutex_guard_base& operator=(const mutex_guard_base& mtx) = delete;

  void swap(mutex_guard_base& other) {
    interlocked_swap_pointer(this->m_mtx, other.m_mtx);
    interlocked_swap(this->m_owned, other.m_owned);
  }

  bool owns() const noexcept { return m_owned; }
  Mutex* mutex() noexcept { return m_mtx; }

 protected:
  constexpr mutex_guard_base() = default;
  mutex_guard_base(Mutex& mtx, bool owned = false)
      : m_mtx{addressof(mtx)}, m_owned{owned} {}

  mutex_guard_base& move_construct_from(mutex_guard_base&& mtx) {
    interlocked_exchange_pointer(
        this->m_mtx, interlocked_exchange_pointer(other.m_mtx, nullptr));
    interlocked_exchange(this->m_owned,
                         interlocked_exchange(other.m_owned, false));
  }

  void lock() {
    m_mtx->lock();
    interlocked_exchange(m_owned, true);
  }

  void lock_shared() {
    m_mtx->lock_shared();
    interlocked_exchange(m_owned, true);
  }

  bool try_lock_for(duration_t timeout_duration) {
    static_assert(has_try_lock_for_v<mutex_t, duration_t>,
                  "Mutex doesn't support locking with timeout");
    interlocked_exchange(m_owned, m_mtx->try_lock_for(timeout_duration));
  }

  void unlock() {
    m_mtx->unlock();
    interlocked_exchange(m_owned, false);
  }

  void unlock_shared() {
    m_mtx->unlock_shared();
    interlocked_exchange(m_owned, false);
  }

  mutex_t* release() {
    mutex_t* mtx{interlocked_exchange_pointer(m_owned, false)};
    interlocked_exchange(m_owned, false);
    return mtx;
  }

 protected:
  mutex_t* m_mtx{nullptr};
  bool m_owned{false};
};
}  // namespace details

template <class Mutex>
class lock_guard : public th::details::mutex_guard_base<Mutex> {
 public:
  using MyBase = th::details::mutex_guard_base<Mutex>;
  using mutex_t = typename MyBase::mutex_t;

 public:
  lock_guard(mutex_t& mtx) : MyBase(mtx) { MyBase::lock(); }
  lock_guard(mutex_t mtx, adopt_lock_tag) : MyBase(mtx, true) {}
  ~lock_guard() { MyBase::unlock(); }
};

template <class Mutex>
lock_guard(Mutex&)
    -> lock_guard<typename th::details::mutex_guard_base<Mutex>::mutex_t>;

template <class Mutex>
lock_guard(Mutex&, adopt_lock_tag)
    -> lock_guard<typename th::details::mutex_guard_base<Mutex>::mutex_t>;

template <class Mutex>
class unique_lock : public th::details::mutex_guard_base<Mutex> {
 public:
  using MyBase = th::details::mutex_guard_base<Mutex>;
  using mutex_t = typename MyBase::mutex_t;
  using duration_t = typename MyBase::duration_t;

 public:
  constexpr unique_lock() = default;
  explicit unique_lock(mutex_t& mtx) : MyBase(mtx) { MyBase::lock(); }
  unique_lock(mutex_t& mtx, adopt_lock_tag) : MyBase(mtx, true) {}
  unique_lock(mutex_t& mtx, defer_lock_tag) : MyBase(mtx) {}

  template <class Mtx = mutex_t,
            enable_if_t<th::details::has_try_lock_for_v<Mtx, duration_t>, int> = 0>
  unique_lock(mutex_t& mtx, duration_t timeout_duration, try_lock_for_tag)
      : MyBase(mtx){MyBase::try_lock_for(timeout_duration)}

        unique_lock(const unique_lock& other) = delete;
  unique_lock operator=(const unique_lock& other) = delete;

  unique_lock(unique_lock&& other) { MyBase::move_construct_from(move(other)); }

  unique_lock operator=(unique_lock&& other) {
    reset();
    MyBase::move_construct_from(move(other));
  }

  ~unique_lock() { reset(); }

  mutex_t* release() noexcept { return MyBase::release(); }
  operator bool() const noexcept { return MyBase::owns(); }

 private:
  void reset() {
    if (MyBase::owns()) {
      MyBase::unlock();
    }
  }
};

template <class Mutex>
unique_lock(Mutex&)
    -> unique_lock<typename th::details::mutex_guard_base<Mutex>::mutex_t>;

template <class Mutex>
unique_lock(Mutex&, adopt_lock_tag)
    -> unique_lock<typename th::details::mutex_guard_base<Mutex>::mutex_t>;

template <class Mutex>
unique_lock(Mutex&, defer_lock_tag)
    -> unique_lock<typename th::details::mutex_guard_base<Mutex>::mutex_t>;

template <class Mutex>
unique_lock(Mutex&,
            typename th::details::mutex_guard_base<Mutex>::duration_t,
            try_lock_for_tag)
    -> unique_lock<typename th::details::mutex_guard_base<Mutex>::mutex_t>;

template <class Mutex>
class shared_lock : public th::details::mutex_guard_base<Mutex> {
 public:
  using MyBase = th::details::mutex_guard_base<Mutex>;
  using mutex_t = typename MyBase::mutex_t;
  using duration_t = typename MyBase::duration_t;

 public:
  constexpr shared_lock() = default;
  explicit shared_lock(mutex_t& mtx) : MyBase(mtx) { MyBase::lock_shared(); }
  shared_lock(mutex_t& mtx, adopt_lock_tag) : MyBase(mtx, true) {}
  shared_lock(mutex_t& mtx, defer_lock_tag) : MyBase(mtx) {}

  template <class Mtx = mutex_t,
            enable_if_t<th::details::has_try_lock_for_v<Mtx, duration_t>, int> = 0>
  shared_lock(mutex_t& mtx, duration_t timeout_duration, try_lock_for_tag)
      : MyBase(mtx){MyBase::try_lock_for(timeout_duration)}

        shared_lock(const shared_lock& other) = delete;
  shared_lock operator=(const shared_lock& other) = delete;

  shared_lock(shared_lock&& other) { MyBase::move_construct_from(move(other)); }

  shared_lock operator=(shared_lock&& other) {
    reset();
    MyBase::move_construct_from(move(other));
  }

  ~shared_lock() { reset(); }

  mutex_t* release() noexcept { return MyBase::release(); }
  operator bool() const noexcept { return MyBase::owns(); }

 private:
  void reset() {
    if (MyBase::owns()) {
      MyBase::unlock_shared();
    }
  }
};

template <class Mutex>
shared_lock(Mutex&)
    -> shared_lock<typename th::details::mutex_guard_base<Mutex>::mutex_t>;

template <class Mutex>
shared_lock(Mutex&, adopt_lock_tag)
    -> shared_lock<typename th::details::mutex_guard_base<Mutex>::mutex_t>;

template <class Mutex>
shared_lock(Mutex&, defer_lock_tag)
    -> shared_lock<typename th::details::mutex_guard_base<Mutex>::mutex_t>;

template <class Mutex>
shared_lock(Mutex&,
            typename th::details::mutex_guard_base<Mutex>::duration_t,
            try_lock_for_tag)
    -> shared_lock<typename th::details::mutex_guard_base<Mutex>::mutex_t>;

// TODO: tuple, scoped_lock
}  // namespace ktl::th
