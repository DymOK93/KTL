#pragma once
#include <basic_types.h>
#include <irql.h>
#include <chrono.hpp>
#include <smart_pointer.hpp>
#include <tuple.hpp>
#include <type_traits.hpp>

namespace ktl {
using thread_id_t = uint32_t;

namespace this_thread {
thread_id_t get_id();

void yield() noexcept;

template <class Rep, class Period>
void sleep_for(const chrono::duration<Rep, Period>& sleep_duration);

template <class Clock, class Duration>
void sleep_until(const chrono::time_point<Clock, Duration>& sleep_time);
}  // namespace this_thread

namespace th::details {
class thread_base : non_copyable {
 public:
  using id = thread_id_t;

  // basic_thread holds a pointer to thread object
  using native_handle_type = PETHREAD;

 private:
  using internal_handle_type = HANDLE;

 public:
  thread_base(thread_base&& other) noexcept;
  thread_base& operator=(thread_base&& other) noexcept;
  ~thread_base() noexcept;

  void swap(thread_base& other) noexcept;

  [[nodiscard]] id get_id() const noexcept;
  [[nodiscard]] native_handle_type native_handle() const noexcept;

  static uint32_t hardware_concurrency() noexcept;

 protected:
  constexpr thread_base() noexcept = default;
  thread_base(internal_handle_type handle);

 private:
  void destroy() const noexcept;

  static native_handle_type obtain_thread_object(internal_handle_type handle);

 private:
  native_handle_type m_thread{};
};

template <irql_t MaxIrql, class Fn, class... Types>
auto pack_fn_with_args(Fn&& fn, Types&&... args) {
  using arg_tuple_t = tuple<decay_t<Fn>, decay_t<Types>...>;
  unique_ptr<arg_tuple_t> packed_args;
  if constexpr (MaxIrql < DISPATCH_LEVEL) {
    packed_args =
        new (paged_new) arg_tuple_t{forward<Fn>(fn), forward<Types>(args)...};
  } else {
    packed_args = new (non_paged_new)
        arg_tuple_t{forward<Fn>(fn), forward<Types>(args)...};
  }
  return packed_args;
}
}  // namespace th::details

class system_thread : th::details::thread_base {};  // Joinable thread

class io_thread : th::details::thread_base {
};  // Supplied by IO-Manager thread prevents the driver from
    // being unloaded before it exits

}  // namespace ktl