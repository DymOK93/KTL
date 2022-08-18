#include <chrono_impl.hpp>

#include <limits.h>

#include <ntddk.h>

namespace ktl::chrono {
namespace details {
[[nodiscard]] intmax_t performance_counter_info::get_frequency()
    const noexcept {
  return m_frequency;
}

intmax_t performance_counter_info::query_perf_counter_freq() noexcept {
  LARGE_INTEGER native_frequency;
  KeQueryPerformanceCounter(&native_frequency);
  return native_frequency.QuadPart;
}

static performance_counter_info perf_counter_info;
}  // namespace details

#if WINVER >= _WIN32_WINNT_WIN8
intmax_t query_system_time() {
  LARGE_INTEGER system_time;
  KeQuerySystemTimePrecise(&system_time);
  return system_time.QuadPart;
}
#elif BITNESS == 32
static constexpr uintptr_t KI_USER_SHARED_DATA{0xFFDF0000};

intmax_t query_system_time() {
  const auto user_shared_data{
      reinterpret_cast<const KUSER_SHARED_DATA*>(KI_USER_SHARED_DATA)};
  const auto& system_time{user_shared_data->SystemTime};
  const uint32_t low_part{ReadNoFence(
      reinterpret_cast<const volatile LONG*>(&system_time.LowPart))};
  return static_cast<intmax_t>(ReadNoFence(&system_time.High1Time))
             << sizeof(uint32_t) * CHAR_BIT |
         low_part;
}
#elif BITNESS == 64
static constexpr uintptr_t KI_USER_SHARED_DATA{0xFFFFF780'00000000};

intmax_t query_system_time() {
  const auto user_shared_data{
      reinterpret_cast<const KUSER_SHARED_DATA*>(KI_USER_SHARED_DATA)};
  const auto raw_system_time{
      reinterpret_cast<const volatile LONG64*>(&user_shared_data->SystemTime)};
  /*
   * KSYSTEM_TIME has no padding between LowPart and High1Time so can be read as
   * a single 64-bit value
   */
  return ReadNoFence64(raw_system_time);
}
#else
#error Unsupported platform
#endif

intmax_t query_performance_counter() {
  const LARGE_INTEGER native_counter{KeQueryPerformanceCounter(nullptr)};
  return native_counter.QuadPart;
}

intmax_t query_performance_counter_frequency() {
  return details::perf_counter_info.get_frequency();
}
}  // namespace ktl::chrono