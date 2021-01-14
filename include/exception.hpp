#pragma once
#include <basic_types.h>
#include <exception_base.h>
#include <placement_new.h>
#include <heap.h>
#include <utility.hpp>

// #include <smart_pointer.hpp>

namespace ktl {
struct exception;
class kernel_error;

struct exception_visitor {
  virtual ~exception_visitor() = default;
  virtual bool visit(const exception& exc) const = 0;
  virtual bool visit(const kernel_error& exc) const = 0;
};

template <class Exc>
struct ignore_single_exception_visitor : exception_visitor {
  using MyBase = exception_visitor;

  using MyBase::visit;
  bool visit(const Exc&) const override { return false; }
};

template <class... Excs>
struct ignore_multiple_exception_visitor
    : public ignore_single_exception_visitor<Excs>... {
  using ignore_single_exception_visitor<Excs>::visit...;
};

using native_char_t = wchar_t;

struct exception : public crt::ExceptionBase {
  // virtual native_char_t* what() const noexcept = 0;
  virtual bool visit(const exception_visitor&) const = 0;
};

template <class ConcreteExc>
struct visitable_exception : exception {
  bool visit(const exception_visitor& exc_visitor) const final {
    return exc_visitor.visit(static_cast<const ConcreteExc&>(*this));
  }
};

class kernel_error : public visitable_exception<kernel_error> {
 public:
  kernel_error(NTSTATUS status) noexcept : m_status{status} {}

  NTSTATUS status() const noexcept { return m_status; }

 private:
  NTSTATUS m_status;
};

namespace exc::details {
template <class Ty, class Deleter>
class smart_ptr {
 public:
  template <class Dx>
  smart_ptr(Ty* ptr, Dx&& deleter)
      : m_ptr{ptr}, m_deleter(forward<Dx>(deleter)) {}

  smart_ptr(smart_ptr&& other)
      : m_ptr{exchange(other.m_ptr, nullptr)},
        m_deleter(move(other.m_deleter)) {}

  smart_ptr& operator=(smart_ptr&& other) {
    if (addressof(other) != this) {
      m_ptr = exchange(other.m_ptr, nullptr);
      m_deleter = move(other.deleter);
    }
    return *this;
  }

  Ty* get() const noexcept { return m_ptr; }
  Ty* release() noexcept { return exchange(m_ptr, nullptr); }

  ~smart_ptr() {
    if (m_ptr) {
      m_deleter(m_ptr);
    }
  }

 private:
  Ty* m_ptr{nullptr};
  Deleter m_deleter;
};

template <class Ty, class Dx>
smart_ptr(Ty, Dx &&) -> smart_ptr<Ty, remove_reference_t<Dx>>;

template <class Exc, class... Types>
auto make_exception(Types&&... args) {
  auto guard{[](void* ptr) { ktl::free(ptr); }};
  exc::details::smart_ptr<void, decltype(guard)> exc_guard(
      ktl::alloc_non_paged(sizeof(Exc)), guard);
  new (exc_guard.get()) Exc(forward<Types>(args)...);
  return exc_guard;
}
}  // namespace exc::details

template <class Exc, class... Types>
[[noreturn]] void ThrowException(Types&&... args) {
  auto exc_guard{exc::details::make_exception<Exc>(forward<Types>(args)...)};
  crt::throw_exception(static_cast<exception*>(exc_guard.release()));
}

namespace exc::details {
inline const exception& get_exception(NTSTATUS exc_code) {
  return static_cast<const exception&>(*crt::get_exception(exc_code));
}

bool handle_exception(NTSTATUS exc_code, const exception_visitor& exc_visitor) {
  const auto& exc{exc::details::get_exception(exc_code)};
  return exc.visit(exc_visitor);
}

template <class ExcHandler>
enable_if_t<!is_convertible_v<ExcHandler, exception_visitor>, bool>
handle_exception(NTSTATUS exc_code, ExcHandler&& handler) {
  return bool_cast(
      forward<Handler>(handler)(exc::details::get_exception(exc_code)));
}

template <class... ExcHandlers>
bool catch_exception_impl(NTSTATUS exc_code, ExcHandlers&&... handlers) {
  return ((handle_exception(exc_code, forward<ExcHandlers>(handlers))) && ...);
}
}  // namespace exc::details

template <class... ExcHandlers>
bool CatchException(NTSTATUS exc_code) {
  return exc::details::catch_exception_impl(exc_code, ExcHandlers{}...);
}

template <class Exc, class... Types>
[[noreturn]] void RethrowException(NTSTATUS old_exc_code, Types&&... args) {
  auto exc_guard{exc::details::make_exception<Exc>(forward<Types>(args)...)};
  crt::replace_exception(old_exc_code,
                         static_cast<exception*>(exc_guard.release()));
}

[[noreturn]] void RethrowException(NTSTATUS old_exc_code) {
  crt::throw_again(old_exc_code);
}

inline NTSTATUS FilterException(NTSTATUS exc_code) {
  if (crt::is_ktl_exception(exc_code)) {
    return EXCEPTION_EXECUTE_HANDLER;
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

template <class... ExcHandlers>
inline NTSTATUS FilterException(NTSTATUS exc_code) {
  if (crt::is_ktl_exception(exc_code) &&
      CatchException<ExcHandlers...>(exc_code)) {
    return EXCEPTION_EXECUTE_HANDLER;
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

}  // namespace ktl
