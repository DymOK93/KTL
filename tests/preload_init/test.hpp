#pragma once

namespace tests::preload_init {
// 1 slot is used by the exception dispatcher
inline constexpr size_t TOTAL_SLOT_COUNT{31};
inline constexpr size_t ACTIVE_SLOT_COUNT{TOTAL_SLOT_COUNT - 1};

void verify_initializers();
}  // namespace tests::preload_init
