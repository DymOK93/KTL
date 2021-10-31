#pragma once
#include <basic_types.hpp>
#include <heap.hpp>

#include <test_runner.hpp>

namespace test::heap {
static constexpr ktl::crt::pool_tag_t POOL_TAG{'eHeT'};

static_assert(ktl::crt::DEFAULT_HEAP_TAG == ktl::crt::KTL_HEAP_TAG);
static_assert(ktl::crt::MEMORY_PAGE_SIZE == PAGE_SIZE);
static_assert(ktl::crt::CACHE_LINE_SIZE == SYSTEM_CACHE_ALIGNMENT_SIZE);
static_assert(static_cast<size_t>(ktl::crt::XMM_ALIGNMENT) >= 16);

#if BITNESS == 64
static_assert(static_cast<size_t>(ktl::crt::DEFAULT_ALLOCATION_ALIGNMENT) ==
              16);
#elif BITNESS == 32
static_assert(static_cast<size_t>(ktl::crt::DEFAULT_ALLOCATION_ALIGNMENT) == 8);
#endif

void run_all(runner& runner);
}



