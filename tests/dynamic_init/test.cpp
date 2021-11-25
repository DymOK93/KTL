#include "test.hpp"

#include <test_runner.hpp>

#include <smart_pointer.hpp>

using namespace ktl;

namespace tests::dynamic_init {
struct OsVersion {
  OsVersion() : version_info{sizeof(OSVERSIONINFOW)} {
    RtlGetVersion(addressof(version_info));
    version_info_copy = make_unique<RTL_OSVERSIONINFOW>(version_info);
  }

  RTL_OSVERSIONINFOW version_info{};
  unique_ptr<RTL_OSVERSIONINFOW> version_info_copy;
};

namespace details {
// Driver Verifier must be enabled to check the destructor
// According to the C++ Standard, an implementation must be guaranteed to
// support registration of at least 32 functions
OsVersion os_version[32];
}  // namespace details

void verify_initializers() {
  for (auto& os_version : details::os_version) {
    auto& version_info{os_version.version_info};
    ASSERT_VALUE(version_info.dwOSVersionInfoSize &&
                 version_info.dwMajorVersion)
  }
}
}  // namespace tests::dynamic_init