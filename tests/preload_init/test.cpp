#include "test.hpp"

#include <test_runner.hpp>

#include <preload_initializer.hpp>
#include <utility_impl.hpp>

using namespace ktl;

namespace tests::preload_init {
struct OsVersion : preload_initializer {
  NTSTATUS run(DRIVER_OBJECT& driver_object,
               UNICODE_STRING& registry_path) noexcept override {
    driver_obj = addressof(driver_object);
    reg_path = addressof(registry_path);
    return STATUS_SUCCESS;
  }

  DRIVER_OBJECT* driver_obj{nullptr};
  UNICODE_STRING* reg_path{nullptr};
};

namespace details {
// clang-format off
OsVersion os_version[ACTIVE_SLOT_COUNT]; // NOLINT(clang-diagnostic-exit-time-destructors)
// clang-format on
}  // namespace details

void verify_initializers() {
  for (auto& os_version : details::os_version) {
    ASSERT_VALUE(os_version.driver_obj && os_version.reg_path)
  }
}
}  // namespace tests::preload_init