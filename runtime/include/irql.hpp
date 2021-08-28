#pragma once
#include <basic_types.h>
#include <ntddk.h>

namespace ktl {
using irql_t = uint8_t;

irql_t get_current_irql();
irql_t raise_irql(irql_t new_irql);
void lower_irql(irql_t new_irql);
bool set_irql(irql_t new_irql);

bool irql_less_or_equal(irql_t target_max_irql);
void verify_max_irql(irql_t target_max_irql);
}  // namespace ktl