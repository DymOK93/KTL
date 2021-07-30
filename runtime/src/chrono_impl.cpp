#include <chrono_impl.h>

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

#else
#if BITNESS == 32
#error 32-bit OS is unsupported now
#elif BITNESS == 64
static constexpr uint64_t KI_USER_SHARED_DATA{0xFFFFF78000000000};
static constexpr uint64_t SHARED_SYSTEM_TIME{KI_USER_SHARED_DATA + 0x14};

intmax_t query_system_time() {
  const auto* system_time_ptr{
      reinterpret_cast<const volatile int64_t*>(SHARED_SYSTEM_TIME)};
  return static_cast<intmax_t>(ReadNoFence64(system_time_ptr));
}
#else
#error Unsupported platform
#endif
#endif

intmax_t query_performance_counter() {
  const LARGE_INTEGER native_counter{KeQueryPerformanceCounter(nullptr)};
  return native_counter.QuadPart;
}

intmax_t query_performance_counter_frequency() {
  return details::perf_counter_info.get_frequency();
}
}  // namespace ktl::chrono