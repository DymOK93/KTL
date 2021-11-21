#pragma once

namespace tests::dynamic_init {
// Driver Verifier must be enabled to check the destructor
// According to the C++ Standard, an implementation must be guaranteed to
// support registration of at least 32 functions
inline constexpr size_t INITIALIZERS_COUNT{32};

void verify_initializers();
}  // namespace tests::heap
