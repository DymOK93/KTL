#include <ntifs.h>

#include <crt_assert.hpp>
#include <exception.hpp>
#include <irql.hpp>
#include <placement_new.hpp>
#include <utility_impl.hpp>

namespace ktl {
namespace crt {
exception_allocator::~exception_allocator() noexcept {
  if (m_buffer) {
    MmFreeNonCachedMemory(m_buffer, RESERVED_BYTES_COUNT);
  }
}

NTSTATUS exception_allocator::run(
    [[maybe_unused]] DRIVER_OBJECT& driver_object,
    [[maybe_unused]] UNICODE_STRING& registry_path) noexcept {
  m_buffer = static_cast<byte*>(
      MmAllocateNonCachedMemory(RESERVED_BYTES_COUNT));
  if (!m_buffer) {
    return STATUS_NO_MEMORY;
  }
  byte* buffer{m_buffer};
  for (auto& current_slot : m_slots) {
    current_slot = buffer;
    buffer += SLOT_SIZE;
  }
  return STATUS_SUCCESS;
}

byte* exception_allocator::allocate(size_t bytes_count) {
  if (bytes_count <= SLOT_SIZE) {
    if (byte* slot = try_get_slot(); slot) {
      return slot;
    }
  }
  return allocate_from_heap(bytes_count);
}

void exception_allocator::deallocate(byte* ptr) noexcept {
  const auto last_slot_begin{m_buffer + RESERVED_BYTES_COUNT - SLOT_SIZE};
  if (const bool not_in_buffer{ptr < m_buffer || ptr > last_slot_begin};
      not_in_buffer) {
    free_to_heap(ptr);
  } else {
    auto& counter{reinterpret_cast<volatile LONG&>(m_current_slot)};
    const auto slot_index{InterlockedDecrement(&counter)};
    m_slots[slot_index] = ptr;
  }
}

byte* exception_allocator::try_get_slot() noexcept {
  auto& counter{reinterpret_cast<volatile LONG&>(m_current_slot)};
  if (const auto slot_index =
          static_cast<size_t>(InterlockedIncrement(&counter) - 1);
      slot_index < SLOT_COUNT) {
    return m_slots[slot_index];
  }
  InterlockedDecrement(&counter);
  return nullptr;
}

byte* exception_allocator::allocate_from_heap(size_t bytes_count) {
  if (get_current_irql() <= DISPATCH_LEVEL) {
    void* const buffer{
        allocate_memory(alloc_request_builder{bytes_count, NonPagedPool}
                            .set_alignment(SLOT_ALIGNMENT)
                            .set_pool_tag(ALLOCATION_TAG)
                            .build())};
    if (buffer) {
      return static_cast<byte*>(buffer);
    }
  }
  throw bad_alloc{};
}

void exception_allocator::free_to_heap(byte* ptr) noexcept {
  crt_assert_with_msg(
      get_current_irql() <= DISPATCH_LEVEL,
      "Deallocating to heap is disabled at IRQL > DISPATCH_LEVEL");
  deallocate_memory(free_request_builder{ptr, SLOT_SIZE}
                        .set_alignment(SLOT_ALIGNMENT)
                        .set_pool_tag(ALLOCATION_TAG)
                        .build());
}

namespace details {
static exception_allocator exc_memory_pool;
}

struct exception_base::shared_data {
  const char* description;
  size_t ref_counter;
};

exception_base::exception_base(const char* msg, size_t msg_length)
    : m_data{make_shared_data(msg, msg_length)} {}

exception_base::exception_base(const wchar_t* msg, size_t msg_length)
    : m_data{construct_from_unicode(msg, msg_length)} {}

exception_base::exception_base(const exception_base& other) noexcept
    : m_data{other.m_data} {
  if (is_shared()) {
    ++get_shared()->ref_counter;
  }
}

exception_base::exception_base(exception_base&& other) noexcept
    : m_data{other.m_data} {
  other.m_data = "";
}

exception_base& exception_base::operator=(
    const exception_base& other) noexcept {
  if (this != &other) {
    exception_base tmp{other};
    swap(tmp);
  }
  return *this;
}

exception_base& exception_base::operator=(exception_base&& other) noexcept {
  if (this != &other) {
    exception_base tmp{move(other)};
    swap(tmp);
  }
  return *this;
}

exception_base::~exception_base() noexcept {
  if (is_shared()) {
    if (auto* shared_data = get_shared(); --shared_data->ref_counter == 0) {
      details::exc_memory_pool.deallocate(reinterpret_cast<byte*>(shared_data));
    }
  }
}

void exception_base::swap(exception_base& other) noexcept {
  ktl::swap(m_data, other.m_data);
}

auto exception_base::construct_from_unicode(const wchar_t* msg, size_t length)
    -> masked_storage_t {
  if (get_current_irql() >= DISPATCH_LEVEL) {
    return UNICODE_CONVERSION_FAILED;
  }
  return make_shared_data(msg, length);
}

const char* exception_base::get_message() const noexcept {
  return is_shared() ? get_shared()->description
                     : static_cast<const char*>(m_data);
}

bool exception_base::is_shared() const noexcept {
  auto* non_const_ptr{const_cast<void*>(m_data)};
  return (ptr_to_number(non_const_ptr) & SHARED_DATA_MASK) == 1;
}

auto exception_base::get_shared() const noexcept -> shared_data* {
  crt_assert_with_msg(is_shared(), "exception hasn't shared data");
  return static_cast<shared_data*>(unmask(const_cast<void*>(m_data)));
}

auto exception_base::make_shared_data(const char* msg, size_t msg_length)
    -> masked_ptr_t {
  byte* buffer{details::exc_memory_pool.allocate(sizeof(shared_data) +
                                                 msg_length + 1ull)};
  auto* msg_buf{reinterpret_cast<char*>(buffer + sizeof(shared_data))};
  memcpy(msg_buf, msg, msg_length);
  return construct_header(buffer, msg_buf, msg_length);
}

auto exception_base::make_shared_data(const wchar_t* msg, size_t msg_length)
    -> masked_ptr_t {
  unsigned long description_length;
  const auto src_length_in_bytes{
      static_cast<unsigned long>(msg_length * sizeof(wchar_t))};
  RtlUnicodeToMultiByteSize(&description_length, msg, src_length_in_bytes);
  byte* buffer{details::exc_memory_pool.allocate(sizeof(shared_data) +
                                                 description_length + 1u)};
  auto* msg_buf{reinterpret_cast<char*>(buffer + sizeof(shared_data))};
  RtlUnicodeToMultiByteN(msg_buf, description_length, nullptr, msg,
                         src_length_in_bytes);
  return construct_header(buffer, msg_buf, src_length_in_bytes);
}

auto exception_base::construct_header(byte* buffer,
                                      char* saved_msg,
                                      size_t msg_length) noexcept
    -> masked_ptr_t {
  saved_msg[msg_length] = static_cast<char>(0);
  auto* data{new (reinterpret_cast<shared_data*>(buffer))
                 shared_data{saved_msg, 1}};
  return mask(data);
}

uintptr_t exception_base::ptr_to_number(void* ptr) noexcept {
  return reinterpret_cast<uintptr_t>(ptr);
}

auto exception_base::mask(void* ptr) noexcept -> masked_ptr_t {
  auto ptr_as_num{ptr_to_number(ptr)};
  ptr_as_num |= SHARED_DATA_MASK;
  return reinterpret_cast<void*>(ptr_as_num);
}

auto exception_base::unmask(void* ptr) noexcept -> masked_ptr_t {
  auto ptr_as_num{ptr_to_number(ptr)};
  ptr_as_num &= ~SHARED_DATA_MASK;
  return reinterpret_cast<void*>(ptr_as_num);
}
}  // namespace crt

const char* exception::what() const noexcept {
  return get_message();
}
NTSTATUS exception::code() const noexcept {
  return STATUS_UNHANDLED_EXCEPTION;
}

NTSTATUS bad_alloc::code() const noexcept {
  return STATUS_NO_MEMORY;
}

NTSTATUS bad_array_new_length::code() const noexcept {
  return STATUS_BUFFER_OVERFLOW;
}
}  // namespace ktl