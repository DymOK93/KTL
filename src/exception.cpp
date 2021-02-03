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
}  // namespace ktl