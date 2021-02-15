#pragma once
#include <basic_types.h>

namespace ktl {
using exc_char_t = wchar_t;

struct constexpr_message_tag {};

namespace crt {
struct exception_data {
  const exc_char_t* str;
  size_t ref_counter;
};

class exception_base {
 private:
  static constexpr size_t SHARED_DATA_MASK{1};
  static constexpr exc_char_t* ALLOCATION_FAILED_MSG{
      L"unable to allocate memory for exception data"};

 public:
  exception_base(const exc_char_t* msg);
  exception_base(const exc_char_t* msg, size_t length);

  template <size_t N>
  explicit constexpr exception_base(const exc_char_t (&msg)[N])
      : exception_base(msg, N) {}

  constexpr exception_base(const exc_char_t* msg,
                           constexpr_message_tag) noexcept
      : m_data{msg} {}

  template <size_t N>
  explicit constexpr exception_base(const exc_char_t (&msg)[N],
                                    constexpr_message_tag)
      : m_data{msg} {}

  exception_base(const exception_base& other);
  exception_base& operator=(const exception_base& other);
  virtual ~exception_base();

 protected:
  const exc_char_t* exception_base::get_message() const noexcept;

 private:
  bool has_shared_data() const noexcept;
  exception_data* as_shared_data() const noexcept;

  static void* try_create_masked_shared_data(const exc_char_t* msg,
                                             size_t msg_length) noexcept;
  static void destroy_shared_data(exception_data* target) noexcept;

 private:
  const void* m_data;  //!< exception_data* or exc_char_t*
};
}  // namespace crt
}  // namespace ktl