#pragma once
#include <basic_types.h>
#include <irql.h>
#include <assert.hpp>
#include <chrono.hpp>
#include <functional.hpp>
#include <smart_pointer.hpp>
#include <tuple.hpp>
#include <type_traits.hpp>

namespace ktl {
using thread_id_t = uint32_t;

namespace this_thread {
thread_id_t get_id();

void yield() noexcept;

template <class Rep, class Period>
void sleep_for(const chrono::duration<Rep, Period>& sleep_duration) {
  const auto tics_to_wait =
      chrono::duration_cast<chrono::tics>(sleep_duration).count();

  if (tics_to_wait < 0) {
    return;
  }

  LARGE_INTEGER interval;
  interval.QuadPart =
      -1 * tics_to_wait;  // A negative value indicates relative time

  KeDelayExecutionThread(KernelMode, false, addressof(interval));
}

template <class Clock, class Duration>
void sleep_until(const chrono::time_point<Clock, Duration>& sleep_time) {
  sleep_for(sleep_time - Clock::now());
}

template <class Rep, class Period>
void stall_for(const chrono::duration<Rep, Period>& stall_duration) {
  const auto us_to_wait =
      chrono::duration_cast<chrono::microseconds>(stall_duration).count();

  assert_with_msg(us_to_wait <= 50,
                  "wait duration in stall_for must not exceed 50 us");

  if (us_to_wait < 0) {
    return;
  }

  KeStallExecutionProcessor(static_cast<unsigned long>(us_to_wait));
}

template <class Clock, class Duration>
void stall_until(const chrono::time_point<Clock, Duration>& stall_time) {
  stall_for(stall_time - Clock::now());
}
}  // namespace this_thread

namespace th::details {
class thread_base : non_copyable {
 public:
  using id = thread_id_t;
  using priority_t = KPRIORITY;

  // basic_thread holds a pointer to thread object
  using native_handle_type = PETHREAD;

 protected:
  using internal_object_type = void*;  // Due to problems with <ntifs.h>

 public:
  thread_base(thread_base&& other) noexcept;
  thread_base& operator=(thread_base&& other) noexcept;
  ~thread_base() noexcept;

  [[nodiscard]] id get_id() const noexcept;

  [[nodiscard]] native_handle_type native_handle() const noexcept {
    return static_cast<native_handle_type>(
        m_thread);  // The real type of PETHREAD depends on included headers
                    // (<ntifs.h>, <fltkrnel.h>)
  }

  priority_t get_priority() const noexcept;
  priority_t set_priority(priority_t new_priority) noexcept;

  static uint32_t hardware_concurrency() noexcept;

 protected:
  constexpr thread_base() noexcept = default;
  thread_base(internal_object_type thread_obj);

  void destroy() noexcept;
  void swap(thread_base& other) noexcept;

  static internal_object_type try_obtain_thread_object(HANDLE handle) noexcept;

 private:
  internal_object_type m_thread{};
};

template <class ConcreteThread>
class worker_thread : public thread_base {
 public:
  using MyBase = thread_base;

 protected:
  using thread_routine_t = void (*)(void*);

 private:
  struct accessor : ConcreteThread {
    using MyBase = ConcreteThread;

    using MyBase::before_exit;
    using MyBase::make_call;
  };

 protected:
  using MyBase::MyBase;

  template <class Fn, class... Types>
  static void make_call(Fn&& fn, Types&&... args) {
    invoke(forward<Fn>(fn), forward<Types>(args)...);
  }

  template <class Fn, class... Types>
  static auto pack_fn_with_args(irql_t max_irql, Fn&& fn, Types&&... args) {
    using arg_tuple_t = tuple<decay_t<Fn>, decay_t<Types>...>;
    unique_ptr<arg_tuple_t> packed_args;
    if (max_irql < DISPATCH_LEVEL) {
      packed_args.reset(new (paged_new) arg_tuple_t{forward<Fn>(fn),
                                                    forward<Types>(args)...});
    } else {
      packed_args.reset(new (non_paged_new) arg_tuple_t{
          forward<Fn>(fn), forward<Types>(args)...});
    }
    return packed_args;
  }

  template <class ArgTuple>
  static constexpr auto get_thread_routine() noexcept {
    return get_thread_routine_impl<ArgTuple>(
        make_index_sequence<tuple_size_v<ArgTuple>>{});
  }

 private:
  template <class ArgTuple, size_t... Indices>
  static constexpr auto get_thread_routine_impl(
      index_sequence<Indices...>) noexcept {
    return &invoke_in_thread<ArgTuple, Indices...>;
  }

  template <class ArgTuple, size_t... Indices>
  static void invoke_in_thread(void* raw_args) noexcept {
    auto* args_ptr{static_cast<ArgTuple*>(raw_args)};
    using invoke_result_t = decltype(
        invoke_with_unpacked_args<ArgTuple, Indices...>(unique_ptr{args_ptr}));
    if constexpr (!is_void_v<invoke_result_t>) {
      accessor::before_exit(invoke_with_unpacked_args<ArgTuple, Indices...>(
          unique_ptr{args_ptr}));
    } else {
      invoke_with_unpacked_args<ArgTuple, Indices...>(unique_ptr{args_ptr});
      accessor::before_exit();
    }
  }

  template <class ArgTuple, size_t... Indices>
  static decltype(auto) invoke_with_unpacked_args(
      unique_ptr<ArgTuple> args_guard) noexcept {
    return accessor::make_call(move(get<Indices>(*args_guard))...);
  }
};
}  // namespace th::details

// Joinable thread
class system_thread : public th::details::worker_thread<system_thread> {
 public:
  using MyBase = worker_thread<system_thread>;

 public:
  constexpr system_thread() noexcept = default;

  template <class Fn, class... Types>
  explicit system_thread(irql_t max_irql, Fn&& fn, Types&&... args)
      : MyBase(
            create_thread(max_irql, forward<Fn>(fn), forward<Types>(args)...)) {
    assert_with_msg(native_handle(),
                    "opening thread failed; thread is running but get_id(), "
                    "join() and detach() are not available");
  }

  template <class Fn, class... Types>
  explicit system_thread(Fn&& fn, Types&&... args)
      : MyBase(create_thread(PASSIVE_LEVEL,
                             forward<Fn>(fn),
                             forward<Types>(args)...)) {
    assert_with_msg(native_handle(),
                    "opening thread failed; thread is running but get_id(), "
                    "join() and detach() are not available");
  }

  system_thread(system_thread&&) noexcept = default;
  system_thread& operator=(system_thread&&) noexcept = default;
  ~system_thread() noexcept;

  bool joinable() const noexcept;
  void join();
  void detach();
  void swap(system_thread& other) noexcept;

 protected:
  template <class Fn, class... Types>
  static NTSTATUS make_call(Fn&& fn, Types&&... args) {
    MyBase::make_call(forward<Fn>(fn), forward<Types>(args)...);
    return STATUS_SUCCESS;  // TODO: check if fn returns NTSTATUS
  }

  static void before_exit(NTSTATUS status) noexcept;

 private:
  template <class Fn, class... Types>
  static internal_object_type create_thread(irql_t max_irql,
                                            Fn&& fn,
                                            Types&&... args) {
    auto packed_args{
        pack_fn_with_args(max_irql, forward<Fn>(fn), forward<Types>(args)...)};
    auto thread_obj{create_thread_impl(
        get_thread_routine<decltype(packed_args)::element_type>(),
        packed_args.get())};
    packed_args.release();
    return thread_obj;
  }

  static internal_object_type create_thread_impl(thread_routine_t start,
                                                 void* raw_args);

  void verify_joinable() const;
};

void swap(system_thread& lhs, system_thread& rhs) noexcept;

// Joinable thread catches all unhandled C++ exceptions
class guarded_system_thread : public system_thread {
 public:
  using MyBase = system_thread;

 public:
  using MyBase::MyBase;

  void swap(guarded_system_thread& other) noexcept;

 protected:
  template <class Fn, class... Types>
  static NTSTATUS make_call(Fn&& fn, Types&&... args) noexcept {
    NTSTATUS status{STATUS_SUCCESS};
    try {
      MyBase::make_call(forward<Fn>(fn), forward<Types>(args)...);
    } catch (const exception& exc) {
      status = exc.code();
    } catch (...) {
      status = STATUS_UNHANDLED_EXCEPTION;
    }
    return status;
  }

 private:
  using MyBase::swap;  // Swapping from guarded_system_thread to system_thread
                       // is allowed, but from system_thread to
                       // guarded_system_thread isn't
};

void swap(guarded_system_thread& lhs, guarded_system_thread& rhs) noexcept;

#if WINVER >= _WIN32_WINNT_WIN8
// Supplied by IO-Manager thread prevents the driver from
// being unloaded before it exits
class io_thread : public th::details::worker_thread<io_thread> {
 public:
  using MyBase = worker_thread<io_thread>;

  struct io_object {
    constexpr io_object(DRIVER_OBJECT* driver_object) noexcept
        : object{driver_object} {}
    constexpr io_object(DEVICE_OBJECT* device_object) noexcept
        : object{device_object} {}

    void* object;
  };

 public:
  constexpr io_thread() noexcept = default;

  template <class Fn, class... Types>
  explicit io_thread(io_object io_obj,
                     irql_t max_irql,
                     Fn&& fn,
                     Types&&... args)
      : MyBase(create_thread(io_obj,
                             max_irql,
                             forward<Fn>(fn),
                             forward<Types>(args)...)) {
    assert_with_msg(native_handle(),
                    "opening thread failed, but thread is running");
  }

  template <class Fn, class... Types>
  explicit io_thread(io_object io_obj, Fn&& fn, Types&&... args)
      : MyBase(create_thread(io_obj,
                             PASSIVE_LEVEL,
                             forward<Fn>(fn),
                             forward<Types>(args)...)) {
    assert_with_msg(native_handle(),
                    "opening thread failed, but thread is running");
  }

  io_thread(io_thread&&) noexcept = default;
  io_thread& operator=(io_thread&&) noexcept = default;

  void swap(io_thread& other) noexcept;

 protected:
  using MyBase::make_call;
  static void before_exit() noexcept;

 private:
  template <class Fn, class... Types>
  static internal_object_type create_thread(io_object io_obj,
                                            irql_t max_irql,
                                            Fn&& fn,
                                            Types&&... args) {
    auto packed_args{
        pack_fn_with_args(max_irql, forward<Fn>(fn), forward<Types>(args)...)};
    auto thread_obj{create_thread_impl(
        io_obj.object,
        get_thread_routine<decltype(packed_args)::element_type>(),
        packed_args.get())};
    packed_args.release();
    return thread_obj;
  }

  static internal_object_type create_thread_impl(void* io_object,
                                                 thread_routine_t start,
                                                 void* raw_args);
};

void swap(io_thread& lhs, io_thread& rhs) noexcept;
#endif
}  // namespace ktl
