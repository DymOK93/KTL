#include <condition_variable.hpp>

namespace ktl {
namespace th::details {
void condition_variable_base::notify_one() noexcept {
  for (;;) {
    size_t current_wait_count{0};
    if (current_wait_count = m_wait_count.load<memory_order_acquire>();
        !current_wait_count) {
      break;
    }
    if (m_wait_count.compare_exchange_strong(current_wait_count,
                                             current_wait_count - 1)) {
      m_event.set();
      break;
    }
  }
}

void condition_variable_base::notify_all() noexcept {
  size_t old_wait_count{m_wait_count.exchange(0)};
  while (old_wait_count--) {
    m_event.set();
  }
}
}  // namespace th::details
}  // namespace ktl