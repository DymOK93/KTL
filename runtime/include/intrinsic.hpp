#pragma once
#include <basic_types.h>

void* memcpy(void* dst, const void* src, size_t size);
#pragma intrinsic(memcpy)

int memcmp(const void* lhs, const void* rhs, size_t size);
#pragma intrinsic(memcmp)



