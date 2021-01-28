#pragma once
#include <crt_attributes.h>
#include <exception_impl.hpp>
#include <utility.hpp>

namespace ktl {
class logic_error : public exception {
 public:
  using MyBase = exception;

 public:
  using MyBase::MyBase;
};

class out_of_range : public logic_error {
 public:
  using MyBase = logic_error;

 public:
  using MyBase::MyBase;
};

class runtime_error : public exception {
 public:
  using MyBase = exception;

 public:
  using MyBase::MyBase;
};

class overflow_error : public runtime_error {
 public:
  using MyBase = runtime_error;

 public:
  using MyBase::MyBase;
};

// Make sure this is not inlined as it is slow and dramatically enlarges code,
// thus making other inlinings more difficult. Throws are also generally the
// slow path.
template <class Exc, class... Types>
[[noreturn]] NOINLINE void throw_exception(Types&&... args) {
  throw Exc(forward<Types>(args)...);
}

namespace exc::details {
template <class Ty>
struct conditional_throw_helper {
  template <class Exc, class... Types>
  static const Ty& throw_exception_if_not(const Ty& cond, Types&&... args) {
    if (!cond) {
      throw_exception<Exc>(forward<Types>(args)...);
    }
    return cond;
  }
};

template <class Ty>
struct conditional_throw_helper<Ty*> {
  template <class Exc, class... Types>
  static Ty* throw_exception_if_not(Ty* ptr, Types&&... args) {
    if (!ptr) {
      throw_exception<Exc>(forward<Types>(args)...);
    }
    return ptr;
  }
};
}  // namespace exc::details

template <class Exc, class Ty, class... Types>
Ty throw_exception_if_not(Ty cond, Types&&... args) {
  return exc::details::conditional_throw_helper<Ty>::throw_exception_if_not<
      Exc>(forward<Ty>(cond), forward<Types>(args)...);
}
}  // namespace ktl