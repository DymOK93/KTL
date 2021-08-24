#include <ntifs.h>

#include <crt_assert.h>
#include <heap.h>
#include <irql.h>
#include <placement_new.h>
#include <char_traits_impl.hpp>
#include <exception.hpp>
#include <utility_impl.hpp>

namespace ktl {
namespace crt {
namespace details {
static exception_allocator exc_memory_pool;
}

exception_allocator::~exception_allocator() noexcept {
  if (m_buffer) {
    MmFreeNonCachedMemory(m_buffer, RESERVED_BYTES_COUNT);
  }
}

NTSTATUS exception_allocator::run(
    [[maybe_unused]] DRIVER_OBJECT& driver_object,
    [[maybe_unused]] UNICODE_STRING& registry_path) noexcept {
  static_assert(SLOT_SIZE % CACHE_LINE_SIZE == 0);
  static_assert(RESERVED_BYTES_COUNT > SLOT_SIZE);

  m_buffer =
      static_cast<byte*>(MmAllocateNonCachedMemory(RESERVED_BYTES_COUNT));
  if (!m_buffer) {
    m_current_slot = SLOT_COUNT;
  } else {
    memset(m_buffer, 0, RESERVED_BYTES_COUNT);
    byte* current_slot{m_buffer};
    for (size_t idx = 0; idx < SLOT_COUNT; ++idx) {
      m_slots[idx] = current_slot;
      current_slot += SLOT_SIZE;
    }
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

void exception_allocator::deallocate(byte* buffer) noexcept {
  if (buffer < m_buffer ||
      buffer > m_buffer + RESERVED_BYTES_COUNT - SLOT_SIZE) {
    deallocate_to_heap(buffer);
  } else {
    auto slot_index{
        InterlockedDecrement(reinterpret_cast<LONG*>(&m_current_slot))};
    m_slots[slot_index] = buffer;
  }
}

byte* exception_allocator::try_get_slot() noexcept {
  byte* buffer{nullptr};
  auto* current_slot_ptr{reinterpret_cast<LONG*>(&m_current_slot)};
  if (auto slot_index = InterlockedIncrement(current_slot_ptr) - 1;
      slot_index < SLOT_COUNT) {
    buffer = m_slots[slot_index];
  } else {
    InterlockedDecrement(current_slot_ptr);
  }
  return buffer;
}

byte* exception_allocator::allocate_from_heap(size_t bytes_count) {
  if (get_current_irql() <= DISPATCH_LEVEL) {
    auto buffer{alloc_non_paged(
        bytes_count, static_cast<std::align_val_t>(CACHE_LINE_SIZE))};
    if (buffer) {
      return static_cast<byte*>(buffer);
    }
  }
  throw bad_alloc{};
}

void exception_allocator::deallocate_to_heap(byte* buffer) noexcept {
  crt_assert_with_msg(get_current_irql() <= DISPATCH_LEVEL,
                      L"Deallocating to heap is disabled at DIRQL");
  free(buffer);
}

struct exception_base::shared_data {
  const char* description;
  size_t ref_counter;
};

exception_base::exception_base(const char* msg, size_t msg_length)
    : m_data{make_shared(msg, msg_length)} {}

exception_base::exception_base(const wchar_t* msg)
    : exception_base(msg, char_traits<wchar_t>::length(msg)) {}

exception_base::exception_base(const wchar_t* msg, size_t msg_length)
    : m_data{construct_from_unicode(msg, msg_length)} {}

exception_base::exception_base(const exception_base& other) noexcept
    : m_data{other.m_data} {
  if (is_shared()) {
    ++get_shared()->ref_counter;
  }
}

exception_base& exception_base::operator=(
    const exception_base& other) noexcept {
  if (this != &other) {
    exception_base tmp{other};
    swap(tmp);
  }
  return *this;
}

exception_base::~exception_base() noexcept {
  if (is_shared()) {
    auto* shared_data{get_shared()};
    --shared_data->ref_counter;
    if (!shared_data->ref_counter) {
      destroy_shared(shared_data);
    }
  }
}

void exception_base::swap(exception_base& other) noexcept {
  ktl::swap(m_data, other.m_data);
}

auto exception_base::construct_from_unicode(const wchar_t* msg, size_t length)
    -> masked_storage_t {
  if (get_current_irql() >= DISPATCH_LEVEL) {
    return CONVERSION_ERROR;
  }
  return convert_to_shared(msg, length);
}

bool exception_base::is_shared() const noexcept {
  auto* non_const_ptr{const_cast<void*>(m_data)};
  return (ptr_to_number(non_const_ptr) & SHARED_DATA_MASK) == 1;
}

auto exception_base::get_shared() const noexcept -> shared_data* {
  crt_assert_with_msg(is_shared(), L"exception hasn't shared data");
  return static_cast<shared_data*>(unmask(const_cast<void*>(m_data)));
}

auto exception_base::make_shared(const char* msg, size_t msg_length)
    -> masked_ptr_t {
  byte* buffer{details::exc_memory_pool.allocate(sizeof(shared_data) +
                                                 msg_length + 1ull)};
  auto* msg_buf{reinterpret_cast<char*>(buffer + sizeof(shared_data))};
  char_traits<char>::copy(msg_buf, msg, msg_length);
  msg_buf[msg_length] = static_cast<char>(0);
  return construct_header(buffer, msg_buf);
}

auto exception_base::convert_to_shared(const wchar_t* msg, size_t msg_length)
    -> masked_ptr_t {
  unsigned long description_length;
  const auto src_length_in_bytes{
      static_cast<unsigned long>(msg_length * sizeof(wchar_t))};
  RtlUnicodeToMultiByteSize(&description_length, msg, src_length_in_bytes);
  byte* buffer{details::exc_memory_pool.allocate(sizeof(shared_data) +
                                                 description_length + 1u)};
  auto* msg_buf{reinterpret_cast<char*>(buffer + sizeof(shared_data))};
  RtlUnicodeToUTF8N(msg_buf, description_length, nullptr, msg,
                    src_length_in_bytes);
  return construct_header(buffer, msg_buf);
}

auto exception_base::construct_header(byte* buffer, char* saved_msg) noexcept
    -> masked_ptr_t {
  auto* data{new (reinterpret_cast<shared_data*>(buffer))
                 shared_data{saved_msg, 1}};
  return mask(data);
}

void exception_base::destroy_shared(masked_ptr_t target) noexcept {
  crt_assert_with_msg((ptr_to_number(target) & SHARED_DATA_MASK) == 1,
                      L"invalid shared data pointer");
  details::exc_memory_pool.deallocate(reinterpret_cast<byte*>(unmask(target)));
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

NTSTATUS bad_alloc::code() const noexcept {
  return STATUS_NO_MEMORY;
}

NTSTATUS bad_array_new_length::code() const noexcept {
  return STATUS_BUFFER_OVERFLOW;
}
}  // namespace ktl