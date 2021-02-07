#include <string.hpp>

namespace ktl {
inline namespace literals {
inline namespace string_literals {
ansi_string operator""_as(const char* str, size_t length) {
  return ansi_string(str, static_cast<ansi_string::size_type>(length));
}

ansi_string_non_paged operator""_asnp(const char* str, size_t length) {
  return ansi_string_non_paged(str,
                               static_cast<ansi_string::size_type>(length));
}

unicode_string operator""_us(const wchar_t* str, size_t length) {
  return unicode_string(str, static_cast<unicode_string::size_type>(length));
}

unicode_string_non_paged operator""_usnp(const wchar_t* str, size_t length) {
  return unicode_string_non_paged(
      str, static_cast<unicode_string::size_type>(length));
}
}  // namespace string_literals
}  // namespace literals
}  // namespace ktl