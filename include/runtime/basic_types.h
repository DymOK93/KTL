#pragma once

#ifndef NO_CXX_STANDARD_LIBRARY
#include <cstdint>
#else
using int8_t = signed char;
using int16_t = short;
using int32_t = int;
using int64_t = long long;
using uint8_t = unsigned char;
using uint16_t = unsigned short;
using uint32_t = unsigned int;
using uint64_t = unsigned long long;;

using size_t = decltype(sizeof(int));

struct nothrow_t {};
inline constexpr nothrow_t nothrow;
#endif

using byte = unsigned char;

