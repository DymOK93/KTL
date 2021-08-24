#include <ktlexcept.hpp>
#include <string.hpp>

namespace ktl {
NTSTATUS out_of_range::code() const noexcept {
  return STATUS_ARRAY_BOUNDS_EXCEEDED;
}

NTSTATUS overflow_error::code() const noexcept {
  return STATUS_BUFFER_OVERFLOW;
}

kernel_error::kernel_error(NTSTATUS code, const char* msg, size_t length)
    : MyBase(msg, length), m_code{code} {}

kernel_error::kernel_error(NTSTATUS code, const wchar_t* msg)
    : MyBase(msg), m_code{code} {}

kernel_error::kernel_error(NTSTATUS code, const wchar_t* msg, size_t length)
    : MyBase(msg, length), m_code{code} {}

kernel_error::kernel_error(NTSTATUS code, const unicode_string& nt_str)
    : MyBase(nt_str), m_code{code} {}

kernel_error::kernel_error(NTSTATUS code, const unicode_string_non_paged& str)
    : MyBase(str), m_code{code} {}

NTSTATUS kernel_error::code() const noexcept {
  return m_code;
}
}  // namespace ktl