#pragma once
#include <basic_types.h>
#include <atomic.hpp>
#include <chrono.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

#include <ntddk.h>

namespace ktl {  // threads management
namespace th::details {
template <class SyncPrimitive>
class sync_primitive_base : non_relocatable {
 public:
  using sync_primitive_t = SyncPrimitive;
  using native_handle_t = sync_primitive_t*;

 public:
  native_handle_t native_handle() noexcept { return addressof(m_native_sp); }

 protected:
  sync_primitive_base() = default;

  native_handle_t get_non_const_native_handle() const noexcept {
    return const_cast<native_handle_t>(addressof(m_native_sp));
  }

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
}  // namespace th::details

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
    : public th::details::sync_primitive_base<FAST_MUTEX> {  // Implemented
                                                             // using
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
  ~shared_mutex() { ExDeleteResourceLite(native_handle()); }

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

// TODO: автонастройка IRQL
class dpc_spin_lock
    : public th::details::sync_primitive_base<KSPIN_LOCK> {  // DISPATCH_LEVEL
                                                             // spin
                                                             // lock
 public:
  using MyBase = th::details::sync_primitive_base<KSPIN_LOCK>;
  using sync_primitive_t = typename MyBase::sync_primitive_t;
  using native_handle_t = typename MyBase::native_handle_t;
  using irql_t = KIRQL;

 public:
  using MyBase = th::details::sync_primitive_base<KSPIN_LOCK>;
  using sync_primitive_t = typename MyBase::sync_primitive_t;
  using native_handle_t = typename MyBase::native_handle_t;

 public:
  dpc_spin_lock() : m_old_irql{KeGetCurrentIrql()} {
    KeInitializeSpinLock(native_handle());
  }

  void lock() { KeAcquireSpinLock(native_handle(), addressof(m_old_irql)); }
  void unlock() { KeReleaseSpinLock(native_handle(), m_old_irql); }

 private:
  irql_t m_old_irql;
};

class semaphore
    : public th::details::sync_primitive_base<KSEMAPHORE> {  // Implemented
                                                             // using
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

namespace th::details {
using event_type = _EVENT_TYPE;
template <event_type type>
class event : public sync_primitive_base<KEVENT> {
 public:
  using MyBase = sync_primitive_base<KEVENT>;
  using sync_primitive_t = typename MyBase::sync_primitive_t;
  using native_handle_t = typename MyBase::native_handle_t;
  using duration_t = chrono::duration_t;

 public:
  enum class State { Active, Inactive };

 public:
  event(State state = State::Inactive) {
    bool activated{state == State::Active};
    KeInitializeEvent(native_handle(), type, activated);
    update_state(state);
  }
  event& operator=(State state) { update_state(state); }

  void wait() {
    KeWaitForSingleObject(native_handle(),
                          Executive,   // Wait reason
                          KernelMode,  // Processor mode
                          false,
                          nullptr  // Indefinite waiting
    );
  }

  void wait_for(duration_t us) {
    LARGE_INTEGER wait_time;
    wait_time.QuadPart = us;
    wait_time.QuadPart *= -10;  // relative time in 100ns tics
    KeWaitForSingleObject(native_handle(),
                          Executive,   // Wait reason
                          KernelMode,  // Processor mode
                          false,
                          addressof(wait_time)  // Indefinite waiting
    );
  }

  void set() noexcept { update_state(State::Active); }
  void clear() noexcept { update_state(State::Inactive); }
  State state() const noexcept { return get_current_state(); }
  bool is_signaled() const noexcept {
    return get_current_state() == State::Active;
  }

 private:
  void update_state(State new_state) noexcept {
    switch (new_state) {
      case State::Active:
        KeSetEvent(native_handle(), 0, false);
        break;
      case State::Inactive:
        KeClearEvent(native_handle());
        break;
      default:
        break;
    };
  }

  State get_current_state() const noexcept {
    return KeReadStateEvent(MyBase::get_non_const_native_handle())
               ? State::Active
               : State::Inactive;
  }
};
}  // namespace th::details

using sync_event =
    th::details::event<th::details::event_type::SynchronizationEvent>;
using notify_event =
    th::details::event<th::details::event_type::NotificationEvent>;

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

}  // namespace th::details

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
class mutex_guard_base : non_copyable {
 public:
  using mutex_t = Mutex;
  using duration_t = get_duration_type_t<mutex_t>;

 public:
  void swap(mutex_guard_base& other) {
    swap(m_mtx, other.m_mtx);
    swap(m_owned, other.m_owned);
  }

  bool owns_lock() const noexcept { return m_owned; }
  Mutex* mutex() noexcept { return m_mtx; }

 protected:
  constexpr mutex_guard_base() noexcept = default;
  mutex_guard_base(Mutex& mtx, bool owned = false) noexcept
      : m_mtx{addressof(mtx)}, m_owned{owned} {}

  mutex_guard_base& move_construct_from(mutex_guard_base&& other) noexcept {
    m_mtx = exchange(other.m_mtx, nullptr);
    m_owned = exchange(other.m_owned, false);
  }

  void lock() {
    m_mtx->lock();
    m_owned = true;
  }

  void lock_shared() {
    m_mtx->lock_shared();
    m_owned = true;
  }

  bool try_lock_for(duration_t timeout_duration) {
    static_assert(has_try_lock_for_v<mutex_t, duration_t>,
                  "Mutex doesn't support locking with timeout");
    m_owned = m_mtx->try_lock_for(timeout_duration);
  }

  void unlock() {
    m_mtx->unlock();
    m_owned = false;
  }

  void unlock_shared() {
    m_mtx->unlock_shared();
    m_owned = false;
  }

  mutex_t* release() {
    m_owned = false;
    return exchange(m_mtx, nullptr);
  }

 protected:
  mutex_t* m_mtx{nullptr};
  bool m_owned{false};
};
}  // namespace th::details

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

  template <
      class Mtx = mutex_t,
      enable_if_t<th::details::has_try_lock_for_v<Mtx, duration_t>, int> = 0>
  unique_lock(mutex_t& mtx, duration_t timeout_duration, try_lock_for_tag)
      : MyBase(mtx) {
    MyBase::try_lock_for(timeout_duration);
  }

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

  template <
      class Mtx = mutex_t,
      enable_if_t<th::details::has_try_lock_for_v<Mtx, duration_t>, int> = 0>
  shared_lock(mutex_t& mtx, duration_t timeout_duration, try_lock_for_tag)
      : MyBase(mtx) {
    MyBase::try_lock_for(timeout_duration);
  }

  shared_lock(const shared_lock& other) = delete;
  shared_lock operator=(const shared_lock& other) = delete;

  shared_lock(shared_lock&& other) { MyBase::move_construct_from(move(other)); }

  shared_lock operator=(shared_lock&& other) {
    reset();
    MyBase::move_construct_from(move(other));
  }

  ~shared_lock() { reset(); }

  mutex_t* release() noexcept { return MyBase::release(); }
  operator bool() const noexcept { return MyBase::owns_lock(); }

 private:
  void reset() {
    if (MyBase::owns_lock()) {
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

// TODO: tuple, scoped_lock
}  // namespace ktl
