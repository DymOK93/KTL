#pragma once
#include <exc_typedefs.h>

namespace ktl::crt::exc_engine {
void stack_walk(record_t& exc_rec);
}