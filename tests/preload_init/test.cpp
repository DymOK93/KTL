#include "test.hpp"

#include <preload_initializer.hpp>
#include <string.hpp>

using namespace ktl;

namespace tests::preload_init {
struct OsVersion : preload_initializer {
  NTSTATUS run(DRIVER_OBJECT& driver_object,
               UNICODE_STRING& registry_path) noexcept {
    driver_obj = addressof(driver_object);
    reg_path = registry_path;
    return STATUS_SUCCESS;
  }

  DRIVER_OBJECT* driver_obj{nullptr};
  unicode_string reg_path;
};

namespace details {
// Driver Verifier must be enabled to check the destructor
// According to the C++ Standard, an implementation must be guaranteed to
// support registration of at least 32 functions
OsVersion os_version[32];
}  // namespace details

void preload_init() {
  for (auto& os_version : details::os_version) {
    ASSERT_VALUE(os_version.driver_obj)
    ASSERT_VALUE(size(os_version.reg_path) && data(os_version.reg_path))
  }
}

void run_all(runner& runner) {
  RUN_TEST(runner, preload_init);
}
}  // namespace tests::preload_init