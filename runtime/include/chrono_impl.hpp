#pragma once
#include <basic_types.hpp>

namespace ktl::chrono {
namespace details {
class performance_counter_info {
 public:
  [[nodiscard]] intmax_t get_frequency() const noexcept;

 private:
  static intmax_t query_perf_counter_freq() noexcept;

 private:
  intmax_t m_frequency{query_perf_counter_freq()};
};
}  // namespace details
intmax_t query_system_time();

intmax_t query_performance_counter();
intmax_t query_performance_counter_frequency();
}  // namespace ktl::chrono