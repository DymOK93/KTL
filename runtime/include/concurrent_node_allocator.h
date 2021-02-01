#pragma once
#include <assert.h>
#include <basic_types.h>
#include <exception_impl.hpp>
#include <heap_impl.hpp>

#include <ntddk.h>

namespace ktl::crt {
class bad_pool : public exception {
 public:
  using MyBase = exception;

 public:
  constexpr bad_pool() noexcept
      : MyBase{L"pool construction fails", constexpr_message_tag{}} {}
};
class concurrent_node_allocator {
 public:
  using lookaside_list_t = LOOKASIDE_LIST_EX;

 public:
  concurrent_node_allocator(memory_pool_type type, size_t entry_size);
  concurrent_node_allocator(const concurrent_node_allocator&) = delete;
  concurrent_node_allocator& operator=(const concurrent_node_allocator&) =
      delete;
  ~concurrent_node_allocator() noexcept;

  void* allocate() noexcept;
  void deallocate(void* node) noexcept;

 private:
  lookaside_list_t m_list;
};
}  // namespace ktl::crt