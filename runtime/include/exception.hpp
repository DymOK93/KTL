#pragma once
#include <basic_types.h>
#include <heap.h>
#include <preload_initializer.h>

#include <ntddk.h>

namespace ktl {
namespace crt {
class exception_allocator : preload_initializer {
  static constexpr size_t SLOT_SIZE{512},  //!< Size of single buffer in bytes
      SLOT_ALIGNMENT{CACHE_LINE_SIZE},     //!< Default alignment of buffer
      RESERVED_PAGES_COUNT{1},             //!< Reserved memory pages count
      RESERVED_BYTES_COUNT{ PAGE_SIZE * RESERVED_PAGES_COUNT},  //!< Reserved memory amount in bytes
      SLOT_COUNT{RESERVED_BYTES_COUNT / SLOT_SIZE};  //!< Buffers count

  static_assert(RESERVED_BYTES_COUNT > SLOT_SIZE);
  static_assert(SLOT_ALIGNMENT >= CACHE_LINE_SIZE);

 public:
  ~exception_allocator() noexcept override;

  NTSTATUS run(DRIVER_OBJECT& driver_object,
               UNICODE_STRING& registry_path) noexcept override;

  byte* allocate(size_t bytes_count);
  void deallocate(byte* ptr) noexcept;

 private:
  byte* try_get_slot() noexcept;

  static byte* allocate_from_heap(size_t bytes_count);
  static void free_to_heap(byte* ptr) noexcept;

 private:
  byte* m_slots[SLOT_COUNT]{};
  byte* m_buffer{nullptr};
  uint32_t m_current_slot{0};
};
}  // namespace crt

struct persistent_message_t {
  explicit persistent_message_t() = default;
};

namespace crt {
class exception_base {
  static constexpr size_t SHARED_DATA_MASK{1};
  static constexpr auto UNICODE_CONVERSION_FAILED{
      "(unable to convert provided Unicode description to ANSI)"};

  struct shared_data;

  using masked_ptr_t = void*;
  using masked_storage_t = const void*;

 public:
  constexpr exception_base() noexcept : m_data{""} {}

 protected:
  exception_base(const char* msg, size_t length);
  exception_base(const wchar_t* msg, size_t length);

  template <size_t N>
  explicit constexpr exception_base(const char (&msg)[N]) noexcept
      : m_data{msg} {}

  explicit constexpr exception_base(persistent_message_t,
                                    const char* msg) noexcept
      : m_data{msg} {}

  exception_base(const exception_base& other) noexcept;
  exception_base(exception_base&& other) noexcept;
  exception_base& operator=(const exception_base& other) noexcept;
  exception_base& operator=(exception_base&& other) noexcept;
  virtual ~exception_base() noexcept;

  void swap(exception_base& other) noexcept;

  [[nodiscard]] const char* get_message() const noexcept;

 private:
  [[nodiscard]] bool is_shared() const noexcept;
  [[nodiscard]] shared_data* get_shared() const noexcept;

  static masked_storage_t construct_from_unicode(const wchar_t* msg,
                                                 size_t length);
  static masked_ptr_t make_shared_data(const char* msg, size_t msg_length);
  static masked_ptr_t make_shared_data(const wchar_t* msg, size_t msg_length);
  static masked_ptr_t construct_header(byte* buffer,
                                       char* saved_msg,
                                       size_t msg_length) noexcept;

  static uintptr_t ptr_to_number(void* ptr) noexcept;
  static masked_ptr_t mask(void* ptr) noexcept;
  static masked_ptr_t unmask(void* ptr) noexcept;

 private:
  masked_storage_t m_data;  //!< const exception_data* or const char*
};
}  // namespace crt

struct exception : crt::exception_base {
  using MyBase = exception_base;

  constexpr exception() noexcept = default;

  [[nodiscard]] virtual const char* what() const noexcept;
  [[nodiscard]] virtual NTSTATUS code() const noexcept;

 protected:
  using MyBase::MyBase;
};

struct bad_alloc : exception {
  using MyBase = exception;

  constexpr bad_alloc() noexcept : MyBase{"memory allocation fails"} {}

  [[nodiscard]] NTSTATUS code() const noexcept override;

 protected:
  using MyBase::MyBase;
};

struct bad_array_new_length : bad_alloc {
  using MyBase = bad_alloc;

  constexpr bad_array_new_length() noexcept
      : MyBase{"array is too long for operator[]"} {}

  [[nodiscard]] NTSTATUS code() const noexcept override;
};
}  // namespace ktl