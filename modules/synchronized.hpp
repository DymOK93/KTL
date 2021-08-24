#pragma once
#include <mutex.h>
#include <type_traits.hpp>
#include <utility.hpp>

namespace ktl {
namespace th::details {
template <class Ty,
          class Mutex,
          template <class>
          class ReadLock,
          template <class>
          class WriteLock>
class synchronized : non_relocatable {
 public:
  using value_type = remove_const_t<remove_reference_t<Ty>>;
  using mutex_type = Mutex;
  using read_guard_type = ReadLock<mutex_type>;
  using write_guard_type = WriteLock<mutex_type>;

  template <class Guard, class Value>
  struct guarded_reference {
    template <class Mtx>
    guarded_reference(Mtx& mtx, Value& value)
        : guard(mtx), ref_to_value(value) {}

    Guard guard;
    Value& ref_to_value;
  };

  using reference = guarded_reference<write_guard_type, value_type>;
  using const_reference = guarded_reference<read_guard_type, const value_type>;

 public:
  template <class U = Ty, enable_if_t<is_default_constructible_v<U>, int> = 0>
  synchronized() noexcept(is_nothrow_default_constructible_v<Ty>) {}

  synchronized& operator=(const Ty& new_value) {
    get_access().ref_to_value = new_value;
    return *this;
  }

  synchronized& operator=(Ty&& new_value) {
    get_access().ref_to_value = move(new_value);
    return *this;
  }

  [[nodiscard]] reference get_write_access() {
    return reference(m_mtx, m_value);
  }

  [[nodiscard]] const_reference get_read_access() const {
    return const_reference(m_mtx, m_value);
  }

  [[nodiscard]] reference get_access() { return get_write_access(); }
  [[nodiscard]] const_reference get_access() const { return get_read_access(); }

 private:
  Ty m_value{};
  mutable mutex_type m_mtx;
};
}  // namespace th::details

template <class Ty>
using synchronized =
    th::details::synchronized<Ty, recursive_mutex, lock_guard, lock_guard>;

template <class Ty>
using synchronized_shared =
    th::details::synchronized<Ty, shared_mutex, shared_lock, lock_guard>;

}  // namespace ktl
