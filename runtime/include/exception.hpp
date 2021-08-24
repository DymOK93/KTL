#pragma once
#include <basic_types.h>
#include <preload_initializer.h>

#include <ntddk.h>

namespace ktl {
struct persistent_message_tag {};

namespace crt {
class exception_allocator : preload_initializer {
  static constexpr size_t SLOT_SIZE{512}, RESERVED_PAGES_COUNT{1},
      RESERVED_BYTES_COUNT{PAGE_SIZE * RESERVED_PAGES_COUNT},
      SLOT_COUNT{RESERVED_BYTES_COUNT / SLOT_SIZE};

 public:
  ~exception_allocator() noexcept;

  NTSTATUS run(DRIVER_OBJECT& driver_object,
               UNICODE_STRING& registry_path) noexcept override;

  byte* allocate(size_t bytes_count);
  void deallocate(byte* buffer) noexcept;

 private:
  byte* try_get_slot() noexcept;

  static byte* allocate_from_heap(size_t bytes_count);
  static void deallocate_to_heap(byte* buffer) noexcept;

 private:
  byte* m_slots[SLOT_COUNT]{};
  byte* m_buffer{nullptr};
  uint32_t m_current_slot{0};
};

class exception_base {
  static constexpr size_t SHARED_DATA_MASK{1};
  static constexpr auto CONVERSION_ERROR{"exception message conversion failed"};

  struct shared_data;

  using masked_ptr_t = void*;
  using masked_storage_t = const void*;

 public:
  exception_base(const char* msg, size_t length);

  exception_base(const wchar_t* msg);
  exception_base(const wchar_t* msg, size_t length);

  template <size_t N>
  explicit constexpr exception_base(const char (&msg)[N]) noexcept
      : m_data{msg} {}

  constexpr exception_base(const char* msg, persistent_message_tag) noexcept
      : m_data{msg} {}

  exception_base(const exception_base& other) noexcept;
  exception_base& operator=(const exception_base& other) noexcept;
  virtual ~exception_base() noexcept;

  void swap(exception_base& other) noexcept;

 protected:
  const char* get_message() const noexcept;

 private:
  bool is_shared() const noexcept;
  shared_data* get_shared() const noexcept;

  static masked_storage_t construct_from_unicode(const wchar_t* msg,
                                                 size_t length);

  static masked_ptr_t make_shared(const char* msg, size_t msg_length);
  static masked_ptr_t convert_to_shared(const wchar_t* msg, size_t msg_length);
  static masked_ptr_t construct_header(byte* buffer, char* saved_msg) noexcept;

  static void destroy_shared(masked_ptr_t target) noexcept;
  static uintptr_t ptr_to_number(void* ptr) noexcept;
  static masked_ptr_t mask(void* ptr) noexcept;
  static masked_ptr_t unmask(void* ptr) noexcept;

 private:
  masked_storage_t m_data;  //!< const exception_data* or const char*
};
}  // namespace crt

struct exception : crt::exception_base {
  using MyBase = exception_base;

  using MyBase::MyBase;
  virtual const char* what() const noexcept;
  virtual NTSTATUS code() const noexcept;
};

struct bad_alloc : public exception {
  using MyBase = exception;

  constexpr bad_alloc() noexcept
      : MyBase{"memory allocation fails", persistent_message_tag{}} {}

  NTSTATUS code() const noexcept override;

 protected:
  using MyBase::MyBase;
};

struct bad_array_new_length : bad_alloc {
  using MyBase = bad_alloc;

  constexpr bad_array_new_length() noexcept
      : MyBase{"array is too long for operator[]"} {}
  NTSTATUS code() const noexcept override;
};
}  // namespace ktl