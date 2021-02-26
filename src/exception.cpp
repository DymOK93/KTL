#include <exception.h>
#include <string.hpp>

namespace ktl {
namespace exc::details {
unicode_string_exception_base::unicode_string_exception_base(
    const unicode_string& str)
    : MyBase(str.data(), str.length()) {}

unicode_string_exception_base::unicode_string_exception_base(
    const unicode_string_non_paged& str)
    : MyBase(str.data(), str.length()) {}
}  // namespace exc::details

NTSTATUS out_of_range::code() const noexcept {
  return STATUS_ARRAY_BOUNDS_EXCEEDED;
}

NTSTATUS overflow_error::code() const noexcept {
  return STATUS_BUFFER_OVERFLOW;
}
}  // namespace ktl