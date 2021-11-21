#include <crt_assert.hpp>
#include <preload_initializer.hpp>

namespace ktl {
preload_initializer::preload_initializer() noexcept {
  [[maybe_unused]] const bool registered{
      preload_initializer_registry::get_instance().add(*this)};
  crt_assert_with_msg(registered, "Registration of preload_initializer failed");
}

preload_initializer_registry&
preload_initializer_registry::get_instance() noexcept {
  static preload_initializer_registry registry;
  return registry;
}

bool preload_initializer_registry::add(preload_initializer& obj) noexcept {
  auto& current_size{m_size};
  if (m_already_ran || current_size > MAX_INITIALIZER_COUNT) {
    return false;
  }
  m_initializers[current_size++] = &obj;
  return true;
}

NTSTATUS preload_initializer_registry::run_all(
    DRIVER_OBJECT& driver_object,
    UNICODE_STRING& registry_path) noexcept {
  m_already_ran = true;
  for (uint32_t idx = 0; idx < m_size; ++idx) {
    auto* entry{m_initializers[idx]};
    if (const NTSTATUS result = entry->run(driver_object, registry_path);
        !NT_SUCCESS(result)) {
      return result;
    }
  }
  return STATUS_SUCCESS;
}
}  // namespace ktl