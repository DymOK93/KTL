#include <exception.hpp>
#include <new_delete.hpp>

#include <ntddk.h>

namespace ktl {
namespace mm::details {
static new_handler_t new_handler;

template <OnAllocationFailure OnFailure>
static void* operator_new_impl(
    size_t bytes_count,
    std::align_val_t alignment,
    crt::pool_type_t pool_type) noexcept(OnFailure !=
                                         OnAllocationFailure::ThrowException) {
  constexpr auto exc_on_failure_masked{
      OnFailure != OnAllocationFailure::ThrowException
          ? OnFailure
          : OnAllocationFailure ::DoNothing};
  for (;;) {
    void* const memory{allocate_memory<exc_on_failure_masked>(
        alloc_request_builder{bytes_count, pool_type}
            .set_alignment(alignment)
            .set_pool_tag(crt::DEFAULT_HEAP_TAG)
            .build())};
    if (memory) {
      return memory;
    }
    if (const auto handler = get_new_handler(); handler) {
      handler();
    } else if constexpr (OnFailure == OnAllocationFailure::ThrowException) {
      throw bad_alloc{};
    }
  }
}

static void operator_delete_impl(
    void* memory,
    size_t bytes_count = 0,
    std::align_val_t alignment = DEFAULT_NEW_ALIGNMENT) noexcept {
  deallocate_memory(free_request_builder{memory, bytes_count}
                        .set_alignment(alignment)
                        .set_pool_tag(crt::DEFAULT_HEAP_TAG)
                        .build());
}

static void operator_delete_impl(void* memory,
                                 std::align_val_t alignment) noexcept {
  operator_delete_impl(memory, 0, alignment);
}
}  // namespace mm::details

new_handler_t get_new_handler() noexcept {
  return static_cast<new_handler_t>(InterlockedCompareExchangePointer(
      reinterpret_cast<volatile PVOID*>(&mm::details::new_handler), nullptr,
      nullptr));
}

new_handler_t set_new_handler(new_handler_t new_h) noexcept {
  return static_cast<new_handler_t>(InterlockedExchangePointer(
      reinterpret_cast<volatile PVOID*>(&mm::details::new_handler), new_h));
}
}  // namespace ktl

void* CRTCALL operator new(size_t bytes_count) {
#ifdef KTL_USING_NON_PAGED_NEW_AS_DEFAULT
  return operator new (bytes_count, ktl::non_paged_new_tag_t{});
#else
  return operator new (bytes_count, ktl::paged_new_tag_t{});
#endif
}

void* CRTCALL operator new(size_t bytes_count, ktl::paged_new_tag_t tag) {
  return operator new(bytes_count, ktl::DEFAULT_NEW_ALIGNMENT, tag);
}

void* CRTCALL operator new(size_t bytes_count, ktl::non_paged_new_tag_t tag) {
  return operator new(bytes_count, ktl::DEFAULT_NEW_ALIGNMENT, tag);
}

void* CRTCALL operator new(size_t bytes_count, std::align_val_t alignment) {
#ifdef KTL_USING_NON_PAGED_NEW_AS_DEFAULT
  return operator new (bytes_count, alignment, ktl::non_paged_new_tag_t{});
#else
  return operator new (bytes_count, alignment, ktl::paged_new_tag_t{});
#endif
}

void* CRTCALL operator new(size_t bytes_count,
                           std::align_val_t alignment,
                           ktl::paged_new_tag_t) {
  return ktl::mm::details::operator_new_impl<
      ktl::OnAllocationFailure::ThrowException>(bytes_count, alignment,
                                                PagedPool);
}

void* CRTCALL operator new(size_t bytes_count,
                           std::align_val_t alignment,
                           ktl::non_paged_new_tag_t) {
  return ktl::mm::details::operator_new_impl<
      ktl::OnAllocationFailure::ThrowException>(bytes_count, alignment,
                                                NonPagedPool);
}

void* CRTCALL operator new(size_t bytes_count,
                           const nothrow_t& nthrw) noexcept {
#ifdef KTL_USING_NON_PAGED_NEW_AS_DEFAULT
  return operator new (bytes_count, nthrw, ktl::non_paged_new_tag_t{});
#else
  return operator new (bytes_count, nthrw, ktl::paged_new_tag_t{});
#endif
}

void* CRTCALL operator new(size_t bytes_count,
                           const nothrow_t& nthrw,
                           ktl::paged_new_tag_t tag) noexcept {
  return operator new(bytes_count, ktl::DEFAULT_NEW_ALIGNMENT, nthrw, tag);
}

void* CRTCALL operator new(size_t bytes_count,
                           const nothrow_t& nthrw,
                           ktl::non_paged_new_tag_t tag) noexcept {
  return operator new(bytes_count, ktl::DEFAULT_NEW_ALIGNMENT, nthrw, tag);
}

void* CRTCALL operator new(size_t bytes_count,
                           std::align_val_t alignment,
                           const nothrow_t& flag) noexcept {
#ifdef KTL_USING_NON_PAGED_NEW_AS_DEFAULT
  return operator new (bytes_count, alignment, flag,
                       ktl::non_paged_new_tag_t{});
#else
  return operator new (bytes_count, alignment, flag, ktl::paged_new_tag_t{});
#endif
}

void* CRTCALL operator new(size_t bytes_count,
                           std::align_val_t alignment,
                           const nothrow_t&,
                           ktl::paged_new_tag_t) noexcept {
  return ktl::mm::details::operator_new_impl<
      ktl::OnAllocationFailure::DoNothing>(bytes_count, alignment, PagedPool);
}

void* CRTCALL operator new(size_t bytes_count,
                           std::align_val_t alignment,
                           const nothrow_t&,
                           ktl::non_paged_new_tag_t) noexcept {
  return ktl::mm::details::operator_new_impl<
      ktl::OnAllocationFailure::DoNothing>(bytes_count, alignment,
                                           NonPagedPool);
}

void CRTCALL operator delete(void* ptr) noexcept {
  ktl::mm::details::operator_delete_impl(ptr);
}

void CRTCALL operator delete(void* ptr, ktl::paged_new_tag_t) noexcept {
  ktl::mm::details::operator_delete_impl(ptr);
}

void CRTCALL operator delete(void* ptr, ktl::non_paged_new_tag_t) noexcept {
  ktl::mm::details::operator_delete_impl(ptr);
}

void CRTCALL operator delete(void* ptr, std::align_val_t alignment) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, alignment);
}

void CRTCALL operator delete(void* ptr,
                             std::align_val_t alignment,
                             ktl::paged_new_tag_t) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, alignment);
}

void CRTCALL operator delete(void* ptr,
                             std::align_val_t alignment,
                             ktl::non_paged_new_tag_t) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, alignment);
}

void CRTCALL operator delete(void* ptr, size_t bytes_count) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, bytes_count);
}
void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             ktl::paged_new_tag_t) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, bytes_count);
}
void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             ktl::non_paged_new_tag_t) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, bytes_count);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             std::align_val_t alignment) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, bytes_count, alignment);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             std::align_val_t alignment,
                             ktl::paged_new_tag_t) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, bytes_count, alignment);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             std::align_val_t alignment,
                             ktl::non_paged_new_tag_t) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, bytes_count, alignment);
}

void CRTCALL operator delete(void* ptr, const nothrow_t&) noexcept {
  ktl::mm::details::operator_delete_impl(ptr);
}

void CRTCALL operator delete(void* ptr,
                             const nothrow_t&,
                             ktl::paged_new_tag_t) noexcept {
  ktl::mm::details::operator_delete_impl(ptr);
}

void CRTCALL operator delete(void* ptr,
                             const nothrow_t&,
                             ktl::non_paged_new_tag_t) noexcept {
  ktl::mm::details::operator_delete_impl(ptr);
}

void CRTCALL operator delete(void* ptr,
                             std::align_val_t alignment,
                             const nothrow_t&) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, alignment);
}

void CRTCALL operator delete(void* ptr,
                             std::align_val_t alignment,
                             const nothrow_t&,
                             ktl::paged_new_tag_t) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, alignment);
}

void CRTCALL operator delete(void* ptr,
                             std::align_val_t alignment,
                             const nothrow_t&,
                             ktl::non_paged_new_tag_t) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, alignment);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             const nothrow_t&) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, bytes_count);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             const nothrow_t&,
                             ktl::paged_new_tag_t) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, bytes_count);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             const nothrow_t&,
                             ktl::non_paged_new_tag_t) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, bytes_count);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             std::align_val_t alignment,
                             const nothrow_t&) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, bytes_count, alignment);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             std::align_val_t alignment,
                             const nothrow_t&,
                             ktl::paged_new_tag_t) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, bytes_count, alignment);
}

void CRTCALL operator delete(void* ptr,
                             size_t bytes_count,
                             std::align_val_t alignment,
                             const nothrow_t&,
                             ktl::non_paged_new_tag_t) noexcept {
  ktl::mm::details::operator_delete_impl(ptr, bytes_count, alignment);
}