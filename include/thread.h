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

 protected:
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

  void destroy() noexcept;

  static native_handle_type obtain_thread_object(internal_handle_type handle);

 private:
  native_handle_type m_thread{};
};

template <class ConcreteThread>
class worker_thread : public thread_base {
 protected:
  using thread_routine_t = void (*)(void*);

 private:
  struct accessor : public ConcreteThread {
    static internal_handle_type start_thread(thread_routine_t start,
                                             void* raw_args) {
      return ConcreteThread::start_thread(start, raw_args);
    }

    template <class Ty>
    static internal_handle_type start_thread(Ty&& special_arg,
                                             thread_routine_t start,
                                             void* raw_args) {
      return ConcreteThread::start_thread(forward<Ty>(special_arg), start,
                                          raw_args);
    }

    template <class ArgTuple, size_t... Indices>
    static decltype(auto) make_call(ArgTuple& arg_tuple,
                                    index_sequence<Indices...>) {
      return ConcreteThread::make_call(move(get<Indices>(arg_tuple))...);
    }

    template <class... ResultTypes>
    static void before_exit(ResultTypes&&... result) {
      ConcreteThread::before_exit(forward<ResultTypes>(result)...);
    }
  };

 public:
  template <class Fn, class... Types>
  worker_thread(irql_t max_irql, Fn&& fn, Types&&... args) {
    auto packed_args{
        pack_fn_with_args(max_irql, forward<Fn>(fn), forward<Types>(args)...)};
    internal_handle_type thread_handle{
        accessor::start_thread(get_thread_routine<>(), packed_args.get()
    )};
  }

  template <class Fn, class... Types>
  worker_thread(Fn&& fn, Types&&... args)
      : worker_thread(PASSIVE_LEVEL, forward<Fn>(fn), forward<Types>(args)...) {
  }

 protected:
  template <class Fn, class... Types>
  static void make_call(Fn&& fn, Types&&... args) {
    // invoke(forward<Fn>(fn), forward<Types>(args)...);
  }

 private:
  template <class Fn, class... Types>
  static auto pack_fn_with_args(irql_t max_irql, Fn&& fn, Types&&... args) {
    using arg_tuple_t = tuple<decay_t<Fn>, decay_t<Types>...>;
    unique_ptr<arg_tuple_t> packed_args;
    if (max_irql < DISPATCH_LEVEL) {
      packed_args =
          new (paged_new) arg_tuple_t{forward<Fn>(fn), forward<Types>(args)...};
    } else {
      packed_args = new (non_paged_new)
          arg_tuple_t{forward<Fn>(fn), forward<Types>(args)...};
    }
    return packed_args;
  }

  template <class ArgTuple, size_t... Indices>
  static constexpr auto get_thread_routine(
      index_sequence<Indices...>) noexcept {
    return &call_with_unpacked_args<ArgTuple, Indices...>;
  }

  template <class ArgTuple, size_t... Indices>
  static void call_with_unpacked_args(
      void* raw_args,
      index_sequence<Indices...> sequence) noexcept {
    const unique_ptr args_guard{static_cast<ArgTuple*>(raw_args)};
    auto& arg_tuple{*args_guard};
    accessor::before_exit(make_call(accessor::make_call(arg_tuple, sequence)));
  }
};
}  // namespace th::details

// Joinable thread
class system_thread : th::details::worker_thread<system_thread> {
 public:
  using MyBase = worker_thread<system_thread>;

  using MyBase::MyBase;

  bool joinable() const noexcept;
  void join();
  void detach();

 protected:
  static internal_handle_type start_thread(thread_routine_t start,
                                           void* raw_args);
  static void before_exit(NTSTATUS status) noexcept;
  using MyBase::make_call;

 private:
  void verify_joinable() const;
};

// Joinable thread enables throwing exceptions
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

 private:
};

// Supplied by IO-Manager thread prevents the driver from
// being unloaded before it exits
class io_thread : th::details::worker_thread<io_thread> {
 public:
  using MyBase = worker_thread<io_thread>;

 protected:
  static void before_exit() noexcept;
  using MyBase::make_call;

 private:
};

}  // namespace ktl