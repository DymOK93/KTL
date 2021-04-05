#include <exception_impl.h>

namespace ktl {
const exc_char_t* exception::what() const noexcept {
  return get_message();
}
NTSTATUS exception::code() const noexcept {
  return STATUS_UNHANDLED_EXCEPTION;
}

NTSTATUS bad_alloc::code() const noexcept {
  return STATUS_NO_MEMORY;
}

constexpr bad_array_length::bad_array_length() noexcept
    : MyBase{L"bad array length", constexpr_message_tag{}} {}

NTSTATUS bad_array_length::code() const noexcept {
  return STATUS_BUFFER_TOO_SMALL;
}
}  // namespace ktl