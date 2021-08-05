#pragma once
#include <basic_types.h>
#include <irql.h>
#include <assert.hpp>
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

 protected:
  using internal_object_type = void*;  // Due to problems with <ntifs.h>

 public:
  thread_base(thread_base&& other) noexcept;
  thread_base& operator=(thread_base&& other) noexcept;
  ~thread_base() noexcept;

  void swap(thread_base& other) noexcept;

  [[nodiscard]] id get_id() const noexcept;

  [[nodiscard]] native_handle_type native_handle() const noexcept {
    return static_cast<native_handle_type>(
        m_thread);  // The real type of PETHREAD depends on included headers
                    // (<ntifs.h>, <fltkrnel.h>)
  }

  static uint32_t hardware_concurrency() noexcept;

 protected:
  constexpr thread_base() noexcept = default;
  thread_base(internal_object_type thread_obj);

  void destroy() noexcept;

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
    // invoke(forward<Fn>(fn), forward<Types>(args)...);
    forward<Fn>(fn)(forward<Types>(args)...);
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
    return &call_with_unpacked_args<ArgTuple, Indices...>;
  }

  template <class ArgTuple, size_t... Indices>
  static void call_with_unpacked_args(void* raw_args) noexcept {
    const unique_ptr args_guard{static_cast<ArgTuple*>(raw_args)};
    auto& arg_tuple{*args_guard};
    using invoke_result_t =
        decltype(accessor::make_call(move(get<Indices>(arg_tuple))...));
    if constexpr (!is_void_v<invoke_result_t>) {
      accessor::before_exit(
          accessor::make_call(move(get<Indices>(arg_tuple))...));
    } else {
      accessor::make_call(move(get<Indices>(arg_tuple))...);
      accessor::before_exit();
    }
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
                    L"opening thread failed; thread is running but get_id(), "
                    L"join() and detach() are not available");
  }

  template <class Fn, class... Types>
  explicit system_thread(Fn&& fn, Types&&... args)
      : MyBase(create_thread(PASSIVE_LEVEL,
                             forward<Fn>(fn),
                             forward<Types>(args)...)) {
    assert_with_msg(native_handle(),
                    L"opening thread failed; thread is running but get_id(), "
                    L"join() and detach() are not available");
  }

  system_thread(system_thread&&) noexcept = default;
  system_thread& operator=(system_thread&&) noexcept = default;
  ~system_thread() noexcept;

  bool joinable() const noexcept;
  void join();
  void detach();

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

// Joinable thread catches all unhandled C++ exceptions
class guarded_system_thread : system_thread {
 public:
  using MyBase = system_thread;

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
};

#if WINVER >= _WIN32_WINNT_WIN8
// Supplied by IO-Manager thread prevents the driver from
// being unloaded before it exits
class io_thread : th::details::worker_thread<io_thread> {
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
                    L"opening thread failed, but thread is running");
  }

  template <class Fn, class... Types>
  explicit io_thread(io_object io_obj, Fn&& fn, Types&&... args)
      : MyBase(create_thread(io_obj,
                             PASSIVE_LEVEL,
                             forward<Fn>(fn),
                             forward<Types>(args)...)) {
    assert_with_msg(native_handle(),
                    L"opening thread failed, but thread is running");
  }

  io_thread(io_thread&&) noexcept = default;
  io_thread& operator=(io_thread&&) noexcept = default;

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
#endif
}  // namespace ktl