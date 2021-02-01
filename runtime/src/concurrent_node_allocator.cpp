#include <concurrent_node_allocator.h>

namespace ktl::crt {
concurrent_node_allocator::concurrent_node_allocator(memory_pool_type type,
                                                     size_t entry_size) {
  if (!NT_SUCCESS(ExInitializeLookasideListEx(&m_list, nullptr, nullptr,
                                              to_native_pool_type(type), 0,
                                              entry_size, 'lkt', 0))) {
    throw bad_pool{};
  }
}

concurrent_node_allocator::~concurrent_node_allocator() noexcept {
  ExDeleteLookasideListEx(&m_list);
}

void* concurrent_node_allocator::allocate() noexcept {
  return ExAllocateFromLookasideListEx(&m_list);
}

void concurrent_node_allocator::deallocate(void* node) noexcept {
  ExFreeToLookasideListEx(&m_list, node);
}
}  // namespace ktl::crt