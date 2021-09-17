#include <irql.hpp>

#include <ntddk.h>

namespace ktl {
irql_t get_current_irql() noexcept {
  return KeGetCurrentIrql();
}

irql_t raise_irql(irql_t new_irql) noexcept {
  irql_t old_irql;
  KeRaiseIrql(new_irql, &old_irql);
  return old_irql;
}

void lower_irql(irql_t new_irql) noexcept {
  KeLowerIrql(new_irql);
}

bool irql_less_or_equal(irql_t target_max_irql) noexcept {
  return get_current_irql() <= target_max_irql;
}

void verify_max_irql(irql_t target_max_irql) noexcept {
  if (!irql_less_or_equal(target_max_irql)) {
    KeBugCheck(IRQL_NOT_LESS_OR_EQUAL);
  }
}
}  // namespace ktl