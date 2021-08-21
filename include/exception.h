#pragma once
#include <crt_attributes.h>
#include <exception_impl.h>
#include <string_fwd.hpp>
#include <utility.hpp>

namespace ktl {
class bad_weak_ptr : public exception {
 public:
  using MyBase = exception;

 public:
  constexpr bad_weak_ptr() noexcept
      : MyBase{L"bad weak ptr", constexpr_message_tag{}} {}
};

namespace exc::details {
class unicode_string_exception_base : public exception {
 public:
  using MyBase = exception;

 public:
  explicit unicode_string_exception_base(const unicode_string& nt_str);
  explicit unicode_string_exception_base(const unicode_string_non_paged& str);

  using MyBase::MyBase;
};
}  // namespace exc::details

class logic_error : public exc::details::unicode_string_exception_base {
 public:
  using MyBase = exc::details::unicode_string_exception_base;

 public:
  using MyBase::MyBase;
};

class out_of_range : public logic_error {
 public:
  using MyBase = logic_error;

 public:
  using MyBase::MyBase;
  NTSTATUS code() const noexcept override;
};

class length_error : public logic_error {
 public:
  using MyBase = logic_error;

 public:
  using MyBase::MyBase;
};

class runtime_error : public exc::details::unicode_string_exception_base {
 public:
  using MyBase = exc::details::unicode_string_exception_base;

 public:
  using MyBase::MyBase;
};

class overflow_error : public runtime_error {
 public:
  using MyBase = runtime_error;

 public:
  using MyBase::MyBase;
  NTSTATUS code() const noexcept override;
};

class kernel_error : public runtime_error {
 public:
  using MyBase = runtime_error;

 public:
  kernel_error(NTSTATUS code, const exc_char_t* msg);
  kernel_error(NTSTATUS code, const exc_char_t* msg, size_t length);

  template <size_t N>
  explicit constexpr kernel_error(NTSTATUS code, const exc_char_t (&msg)[N])
      : MyBase(msg), m_code{code} {}

  constexpr kernel_error(NTSTATUS code,
                         const exc_char_t* msg,
                         constexpr_message_tag tag) noexcept
      : MyBase(msg, tag), m_code{code} {}

  explicit kernel_error(NTSTATUS code, const unicode_string& nt_str);
  explicit kernel_error(NTSTATUS code, const unicode_string_non_paged& str);

  NTSTATUS code() const noexcept override;

 private:
  NTSTATUS m_code;
};

// Make sure this is not inlined as it is slow and dramatically enlarges code,
// thus making other inlinings more difficult. Throws are also generally the
// slow path.
template <class Exc, class... Types>
[[noreturn]] NOINLINE void throw_exception(Types&&... args) {
  throw Exc(forward<Types>(args)...);
}

template <class Exc, class Ty, class... Types>
static void throw_exception_if(const Ty& cond, Types&&... args) {
  if (cond) {
    throw_exception<Exc>(forward<Types>(args)...);
  }
}

template <class Exc, class Ty, class... Types>
static void throw_exception_if_not(const Ty& cond, Types&&... args) {
  if (!cond) {
    throw_exception<Exc>(forward<Types>(args)...);
  }
}
}  // namespace ktl