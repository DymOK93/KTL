#pragma once

namespace std {
enum class align_val_t : size_t {
};  // Предопределённый тип - опредлелить и использовать только
    // ktl::align_val_t{} невозможно
}

namespace ktl {
using byte = unsigned char;
using std::align_val_t;
}  // namespace ktl

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <cstdint>
using std::nothrow_t;
#else

using int8_t = signed char;
using int16_t = short;
using int32_t = int;
using uint8_t = unsigned char;
using uint16_t = unsigned short;
using uint32_t = unsigned int;

#ifdef _M_AMD64
using int64_t = long long;
using uint64_t = unsigned long long;
using intmax_t = int64_t;
using uintmax_t = uint64_t;
#define BITNESS 64
#elif defined(_M_IX86)
using intmax_t = int32_t;
using uintmax_t = uint32_t;
#define BITNESS 32
#else
#error Unsupported platform
#endif

using size_t = decltype(sizeof(int));

using nullptr_t = decltype(nullptr);
using ptrdiff_t = decltype(static_cast<unsigned char*>(nullptr) -
                           static_cast<unsigned char*>(nullptr));

struct nothrow_t {};
inline constexpr nothrow_t nothrow;

using max_align_t = double;  // Most aligned type

#define INT8_C(x) (x)
#define INT16_C(x) (x)
#define INT32_C(x) (x)
#define INT64_C(x) (x##LL)

#define UINT8_C(x) (x)
#define UINT16_C(x) (x)
#define UINT32_C(x) (x##U)
#define UINT64_C(x) (x##ULL)

#define INTMAX_C(x) INT64_C(x)
#define UINTMAX_C(x) UINT64_C(x)
#endif
