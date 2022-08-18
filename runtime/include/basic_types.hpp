#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <cstdint>
using std::nothrow_t;
using std::size_t;
#else
namespace std {  // NOLINT(cert-dcl58-cpp)
/*
 * align_val_t is a compiler-predefined type that must be declared in the std::
 */
enum class align_val_t : size_t {};
}  // namespace std

using int8_t = signed char;
using int16_t = short;
using int32_t = int;
using int64_t = long long;
using uint8_t = unsigned char;
using uint16_t = unsigned short;
using uint32_t = unsigned int;
using uint64_t = unsigned long long;

using int_least8_t = signed char;
using int_least16_t = short;
using int_least32_t = int;
using int_least64_t = long long;
using uint_least8_t = unsigned char;
using uint_least16_t = unsigned short;
using uint_least32_t = unsigned int;
using uint_least64_t = unsigned long long;

using int_fast8_t = signed char;
using int_fast16_t = int;
using int_fast32_t = int;
using int_fast64_t = long long;
using uint_fast8_t = unsigned char;
using uint_fast16_t = unsigned int;  // As if in MSVC headers
using uint_fast32_t = unsigned int;
using uint_fast64_t = unsigned long long;

using intmax_t = long long;
using uintmax_t = unsigned long long;
using size_t = decltype(sizeof(int));
using nullptr_t = decltype(nullptr);

#ifdef _M_AMD64
#define BITNESS 64  // NOLINT(cppcoreguidelines-macro-usage)
using ptrdiff_t = __int64;  // NOLINT(clang-diagnostic-language-extension-token)
using intptr_t = __int64;   // NOLINT(clang-diagnostic-language-extension-token)
using uintptr_t =
    unsigned __int64;  // NOLINT(clang-diagnostic-language-extension-token)
#elif defined(_M_IX86)  // NOLINT(cppcoreguidelines-macro-usage)
#define BITNESS 32
using ptrdiff_t = int;
using intptr_t = int;
using uintptr_t = unsigned int;
#else
#error Unsupported platform
#endif

struct nothrow_t {};
inline constexpr nothrow_t nothrow;

using max_align_t = double;  // Most aligned type

#define INT8_C(x) (x)  // NOLINT(cppcoreguidelines-macro-usage)
#define INT16_C(x) (x)  // NOLINT(cppcoreguidelines-macro-usage)
#define INT32_C(x) (x)  // NOLINT(cppcoreguidelines-macro-usage)
#define INT64_C(x) (x##LL)  // NOLINT(cppcoreguidelines-macro-usage)
#define UINT8_C(x) (x)  // NOLINT(cppcoreguidelines-macro-usage)
#define UINT16_C(x) (x)  // NOLINT(cppcoreguidelines-macro-usage)
#define UINT32_C(x) (x##U)  // NOLINT(cppcoreguidelines-macro-usage)
#define UINT64_C(x) (x##ULL)  // NOLINT(cppcoreguidelines-macro-usage)

#define INTMAX_C(x) INT64_C(x)  // NOLINT(cppcoreguidelines-macro-usage)
#define UINTMAX_C(x) UINT64_C(x)  // NOLINT(cppcoreguidelines-macro-usage)
#endif

namespace ktl {
using byte = unsigned char;
using ::nullptr_t;
using std::align_val_t;
}  // namespace ktl
