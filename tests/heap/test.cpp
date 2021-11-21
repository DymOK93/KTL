#include "test.hpp"

#include <smart_pointer.hpp>

#include <test_runner.hpp>

using namespace ktl;

namespace tests::heap {
namespace details {
template <size_t BytesCount,
          align_val_t Align,
          crt::pool_type_t PoolType,
          OnAllocationFailure OnFailure,
          bool RepeatIfFailed,
          class Predicate>
void alloc_and_free_impl(Predicate pred) {
  constexpr bool throw_on_failure{
      !RepeatIfFailed && OnFailure == OnAllocationFailure::ThrowException};

  const void* ptr{nullptr};
  bool unexpected_throw{false};

  try {
    constexpr auto request{alloc_request_builder{BytesCount, PoolType}
                               .set_alignment(Align)
                               .set_pool_tag(POOL_TAG)
                               .build()};

    unique_ptr ptr_guard{PVOID{}, [](void* p) {
                           deallocate_memory(free_request_builder{p, BytesCount}
                                                 .set_pool_tag(POOL_TAG)
                                                 .build());
                         }};

    if constexpr (!RepeatIfFailed) {
      ptr_guard.reset(allocate_memory<OnFailure>(request));
    } else {
      while (!ptr_guard) {
        ptr_guard.reset(allocate_memory<OnFailure>(request));
      }
    }

    ptr = ptr_guard.get();

  } catch (...) {
    unexpected_throw = true;
  }
  ASSERT_VALUE(pred(ptr))
  ASSERT_EQ(unexpected_throw, throw_on_failure && ptr == nullptr)
}

template <align_val_t Align>
auto make_align_checker() {
  return [](const void* p) {
    constexpr auto alignment{static_cast<size_t>(Align)};
    const auto lower_bits{reinterpret_cast<uintptr_t>(p) & (alignment - 1)};
    return lower_bits == 0;
  };
}
}  // namespace details

#define CHECK_ALLOC_AND_FREE(FailurePolicy)                                  \
  details::alloc_and_free_impl<                                              \
      (numeric_limits<size_t>::max)(), crt::CACHE_LINE_ALLOCATION_ALIGNMENT, \
      NonPagedPoolNx, OnAllocationFailure::##FailurePolicy, false>(          \
      [](const void* p) { return p == nullptr; });                           \
                                                                             \
  details::alloc_and_free_impl<128, crt::DEFAULT_ALLOCATION_ALIGNMENT,       \
                               PagedPool,                                    \
                               OnAllocationFailure::##FailurePolicy, true>(  \
      details::make_align_checker<crt::DEFAULT_ALLOCATION_ALIGNMENT>());     \
                                                                             \
  details::alloc_and_free_impl<128, crt::CACHE_LINE_ALLOCATION_ALIGNMENT,    \
                               PagedPool,                                    \
                               OnAllocationFailure::##FailurePolicy, true>(  \
      details::make_align_checker<crt::CACHE_LINE_ALLOCATION_ALIGNMENT>());

void alloc_and_free() {
  CHECK_ALLOC_AND_FREE(ThrowException)
}

void alloc_and_free_noexcept() {
  CHECK_ALLOC_AND_FREE(DoNothing)
}
}  // namespace tests::heap
