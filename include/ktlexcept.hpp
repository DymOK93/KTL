#pragma once
#include <crt_attributes.h>
#include <exception.hpp>
#include <string_fwd.hpp>
#include <utility.hpp>

namespace ktl {
class bad_weak_ptr : public exception {
 public:
  using MyBase = exception;

 public:
  constexpr bad_weak_ptr() noexcept
      : MyBase{"bad weak ptr"} {}
};

namespace exc::details {
struct winnt_string_exception_base : exception {
  using MyBase = exception;

  template <typename CharT, size_t SsoChBufferCount, class Traits, class Alloc>
  explicit winnt_string_exception_base(
      const basic_winnt_string<CharT, SsoChBufferCount, Traits, Alloc>& str)
      : MyBase(data(str), size(str)) {}

  using MyBase::MyBase;
};
}  // namespace exc::details

struct logic_error : exc::details::winnt_string_exception_base {
  using MyBase = winnt_string_exception_base;
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

struct runtime_error : exc::details::winnt_string_exception_base {
  using MyBase = winnt_string_exception_base;
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
  kernel_error(NTSTATUS code, const char* msg, size_t length);

  kernel_error(NTSTATUS code, const wchar_t* msg);
  kernel_error(NTSTATUS code, const wchar_t* msg, size_t length);

  constexpr kernel_error(NTSTATUS code,
                         const char* msg,
                         persistent_message_tag tag) noexcept
      : MyBase(msg, tag), m_code{code} {}

  template <size_t N>
  explicit constexpr kernel_error(NTSTATUS code, const char (&msg)[N])
      : MyBase(msg), m_code{code} {}

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