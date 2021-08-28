#include <irql.hpp>

#include <ntddk.h>

namespace ktl {
irql_t get_current_irql() {
  return KeGetCurrentIrql();
}

irql_t raise_irql(irql_t new_irql) {
  irql_t old_irql;
  KeRaiseIrql(new_irql, &old_irql);
  return old_irql;
}

void lower_irql(irql_t new_irql) {
  KeLowerIrql(new_irql);
}

bool set_irql(irql_t new_irql) {
  auto current_irql { get_current_irql() };
  if (new_irql == current_irql) {
    return false;
  }
  if (new_irql > current_irql) {
    raise_irql(new_irql);
  }
  else if (new_irql < current_irql) {
    lower_irql(new_irql);
  }
  return true;
}

bool irql_less_or_equal(irql_t target_max_irql) {
  return target_max_irql <= get_current_irql();
}

void verify_max_irql(irql_t target_max_irql) {
  if (get_current_irql() > target_max_irql) {
    KeBugCheck(IRQL_NOT_LESS_OR_EQUAL);
  }
}
}  // namespace ktl