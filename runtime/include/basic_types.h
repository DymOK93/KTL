#pragma once

namespace ktl {
using byte = unsigned char;
}

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
#elif defined(_M_IX86)
using intmax_t = int32_t;
using uintmax_t = uint32_t;
#else
#error Unknown platform
#endif

using size_t = decltype(sizeof(int));

using nullptr_t = decltype(nullptr);
using ptrdiff_t = decltype(static_cast<unsigned char*>(nullptr) -
                           static_cast<unsigned char*>(nullptr));

struct nothrow_t {};
inline constexpr nothrow_t nothrow;

using max_align_t = double;  // Most aligned type
#endif
