#include <preload_initializer.h>

namespace ktl::crt {
preload_initializer::preload_initializer() noexcept {
  preload_initializer_registry::get_instance().add(*this);
}

preload_initializer_registry&
preload_initializer_registry::get_instance() noexcept {
  static preload_initializer_registry preload_reg;
  return preload_reg;
}

bool preload_initializer_registry::add(preload_initializer& obj) noexcept {
  auto& current_size{m_size};
  if (current_size > MAX_INITIALIZER_COUNT) {
    return false;
  }
  m_initializers[current_size++] = &obj;
  return true;
}

NTSTATUS preload_initializer_registry::run_all(
    DRIVER_OBJECT& driver_object,
    UNICODE_STRING& registry_path) noexcept {
  for (uint32_t idx = 0; idx < m_size; ++idx) {
    auto* entry{m_initializers[idx]};
    NTSTATUS result{entry->run(driver_object, registry_path)};
    if (!NT_SUCCESS(result)) {
      return result;
    }
  }
  return STATUS_SUCCESS;
}

}  // namespace ktl::crt