#pragma once
#include <atomic.hpp>
#include <chrono.hpp>
#include <mutex.h>
#include <type_traits.hpp>
#include <utility.hpp>

namespace ktl {
namespace th::details {
template <class Lockable, class = void>
struct has_lock : false_type {};

template <class Lockable>
struct has_lock<Lockable, void_t<decltype(declval<Lockable>().unlock())>>
    : true_type {};

template <class Lockable, class = void>
struct has_unlock : false_type {};

template <class Lockable>
struct has_unlock<Lockable, void_t<decltype(declval<Lockable>().unlock())>>
    : true_type {};

template <class Lockable>
inline constexpr bool is_basic_lockable_v =
    has_lock<Lockable>::value&& has_unlock<Lockable>::value;

template <class Lockable>
struct basic_lockable_checker {
  static_assert(is_basic_lockable_v<Lockable>,
                "Lockable doesn't satisfy requirements of BasicLockable");
};

class condition_variable_base : non_relocatable {
 public:
  void notify_one() noexcept;
  void notify_all() noexcept;

 protected:
  sync_event m_event;
  atomic_size_t m_wait_count{0};
};
}  // namespace th::details

struct condition_variable_any : public th::details::condition_variable_base {
  template <class Lockable>
  void wait(Lockable& lock) {
    th::details::basic_lockable_checker<Lockable>{};
    lock.unlock();
    ++m_wait_count;
    m_event.wait();
    lock.lock();
  }

  template <class Lockable, class Predicate>
  void wait(Lockable& lock, Predicate pred) {
    while (!pred) {
      wait(lock);
    }
  }
};

struct condition_variable : public th::details::condition_variable_base {
  template <class Mutex>
  void wait(unique_lock<Mutex>& lock) {
    lock.unlock();
    ++m_wait_count;
    m_event.wait();
    lock.lock();
  }

  template <class Mutex, class Predicate>
  void wait(unique_lock<Mutex>& lock, Predicate pred) {
    while (!pred) {
      wait(lock);
    }
  }
};
}  // namespace ktl