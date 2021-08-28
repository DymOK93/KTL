#pragma once
#include <crt_attributes.hpp>
#include <char_traits.hpp>
#include <exception.hpp>
#include <string_fwd.hpp>
#include <utility.hpp>

namespace ktl {
namespace exc::details {
struct winnt_exception_base : exception {
  using MyBase = exception;

  template <class Traits = char_traits<char>>
  winnt_exception_base(const basic_ansi_string_view<Traits>& str)
      : MyBase(data(str), size(str)) {}

  template <class Traits = char_traits<wchar_t>>
  winnt_exception_base(const basic_unicode_string_view<Traits>& str)
      : MyBase(data(str), size(str)) {}

  template <class Traits = char_traits<char>>
  constexpr winnt_exception_base(persistent_message_t,
                                 const basic_ansi_string_view<Traits>& str)
      : MyBase(persistent_message_t{}, data(str)) {}

  template <size_t N>
  constexpr winnt_exception_base(const char (&str)[N]) : MyBase(str) {}
};
}  // namespace exc::details

struct logic_error : exc::details::winnt_exception_base {
  using MyBase = winnt_exception_base;
  using MyBase::MyBase;
};

struct invalid_argument : logic_error {
  using MyBase = logic_error;
  using MyBase::MyBase;
};

struct domain_error : logic_error {
  using MyBase = logic_error;
  using MyBase::MyBase;
};

struct length_error : logic_error {
  using MyBase = logic_error;
  using MyBase::MyBase;
};

struct out_of_range : logic_error {
  using MyBase = logic_error;
  using MyBase::MyBase;
  [[nodiscard]] NTSTATUS code() const noexcept override;
};

struct future_error : logic_error {
  using MyBase = logic_error;
  constexpr future_error() noexcept
      : MyBase{"future has no associated shared state"} {}
  [[nodiscard]] NTSTATUS code() const noexcept override;
};

struct runtime_error : exc::details::winnt_exception_base {
  using MyBase = winnt_exception_base;
  using MyBase::MyBase;
};

struct range_error : runtime_error {
  using MyBase = runtime_error;
  using MyBase::MyBase;
  [[nodiscard]] NTSTATUS code() const noexcept override;
};

struct overflow_error : runtime_error {
  using MyBase = runtime_error;
  using MyBase::MyBase;
  [[nodiscard]] NTSTATUS code() const noexcept override;
};

struct underflow_error : runtime_error {
  using MyBase = runtime_error;
  using MyBase::MyBase;
  [[nodiscard]] NTSTATUS code() const noexcept override;
};

struct kernel_error : runtime_error {
  using MyBase = runtime_error;

  template <class Traits = char_traits<char>>
  kernel_error(NTSTATUS code, const basic_ansi_string_view<Traits>& str)
      : MyBase(str), m_code{code} {}

  template <class Traits = char_traits<wchar_t>>
  kernel_error(NTSTATUS code, const basic_unicode_string_view<Traits>& str)
      : MyBase(str), m_code{code} {}

  template <class Traits = char_traits<char>>
  constexpr kernel_error(persistent_message_t,
                         NTSTATUS code,
                         const basic_ansi_string_view<Traits>& str)
      : MyBase(persistent_message_t{}, str), m_code{code} {}

  template <size_t N>
  constexpr kernel_error(NTSTATUS code, const char (&str)[N])
      : MyBase(str), m_code{code} {}

  [[nodiscard]] NTSTATUS code() const noexcept override;

 private:
  NTSTATUS m_code;
};

struct format_error : runtime_error {
  using MyBase = runtime_error;
  using MyBase::MyBase;
  [[nodiscard]] NTSTATUS code() const noexcept override;
};

// Make sure this is not inlined as it is slow and dramatically enlarges code,
// thus making other inlinings more difficult. Throws are also generally the
// slow path
template <class Exc, class... Types>
[[noreturn]] NOINLINE void throw_exception(Types&&... args) {
  throw Exc(forward<Types>(args)...);
}

template <class Exc, class Ty, class... Types>
FORCEINLINE void throw_exception_if(const Ty& cond, Types&&... args) {
  if (cond) {
    throw_exception<Exc>(forward<Types>(args)...);
  }
}

template <class Exc, class Ty, class... Types>
FORCEINLINE void throw_exception_if_not(const Ty& cond, Types&&... args) {
  if (!cond) {
    throw_exception<Exc>(forward<Types>(args)...);
  }
}
}  // namespace ktl