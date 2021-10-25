#include <ktlexcept.hpp>
#include <string.hpp>

namespace ktl {
NTSTATUS out_of_range::code() const noexcept {
  return STATUS_ARRAY_BOUNDS_EXCEEDED;
}

NTSTATUS range_error::code() const noexcept {
  return STATUS_RANGE_NOT_FOUND;
}

NTSTATUS overflow_error::code() const noexcept {
  return STATUS_BUFFER_OVERFLOW;
}

NTSTATUS kernel_error::code() const noexcept {
  return m_code;
}

NTSTATUS format_error::code() const noexcept {
  return STATUS_BAD_DESCRIPTOR_FORMAT;
}
}  // namespace ktl