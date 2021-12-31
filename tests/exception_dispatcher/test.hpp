#pragma once

namespace tests::exception_dispatcher {
void throw_directly();
void throw_in_nested_call();
void throw_on_array_init();
void throw_in_new_expression();
void throw_at_high_irql();

void catch_by_value();
void catch_by_base_ref();
void catch_by_base_ptr();
}  // namespace tests::exception_dispatcher
