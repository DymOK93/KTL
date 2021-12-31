#include "test.hpp"

#include <test_runner.hpp>

#include <irql.hpp>

using namespace ktl;

namespace tests::irql {
void raise_and_lower() {
  const irql_t prev{raise_irql(DISPATCH_LEVEL)};
  ASSERT_EQ(prev, PASSIVE_LEVEL)
  ASSERT_EQ(DISPATCH_LEVEL, KeGetCurrentIrql())
  lower_irql(prev);
  ASSERT_EQ(prev, KeGetCurrentIrql())
}

void current() {
  ASSERT_EQ(get_current_irql(), KeGetCurrentIrql())
  const irql_t prev{raise_irql(DISPATCH_LEVEL)};
  ASSERT_EQ(get_current_irql(), KeGetCurrentIrql())
  lower_irql(prev);
  ASSERT_EQ(get_current_irql(), KeGetCurrentIrql())
}

void less_or_equal() {
  ASSERT_VALUE(irql_less_or_equal(APC_LEVEL))
  const irql_t prev{raise_irql(DISPATCH_LEVEL)};
  ASSERT_VALUE(!irql_less_or_equal(APC_LEVEL))
  lower_irql(prev);
  ASSERT_VALUE(irql_less_or_equal(APC_LEVEL))
}
}  // namespace tests::irql
