#include <bugcheck.h>
#include <exception_base.h>
#include <heap.h>
#include <placement_new.h>
#include <char_traits_impl.hpp>

namespace ktl::crt {
exception_base::exception_base(const exc_char_t* msg)
    : m_data{create_masked_shared_data(msg,
                                       char_traits<exc_char_t>::length(msg))} {}

exception_base::exception_base(const exc_char_t* msg, size_t msg_length)
    : m_data{create_masked_shared_data(msg, msg_length)} {}

exception_base::exception_base(const exception_base& other)
    : m_data{other.m_data} {
  if (has_shared_data()) {
    ++as_shared_data()->ref_counter;
  }
}

exception_base& exception_base::operator=(const exception_base& other) {
  if (this != &other) {
    m_data = other.m_data;
    if (has_shared_data()) {
      ++as_shared_data()->ref_counter;
    }
  }
  return *this;
}

exception_base::~exception_base() {
  if (has_shared_data()) {
    auto* shared_data{as_shared_data()};
    --shared_data->ref_counter;
    if (!shared_data->ref_counter) {
      destroy_shared_data(shared_data);
    }
  }
}

bool exception_base::has_shared_data() const noexcept {
  return reinterpret_cast<size_t>(m_data) &
         SHARED_DATA_MASK;  // Адреса выровнены по границе 16 байт -
                            // можно использовать младшие биты
}

exception_data* exception_base::as_shared_data() const noexcept {
  return reinterpret_cast<exception_data*>(reinterpret_cast<size_t>(m_data) &
                                           ~SHARED_DATA_MASK);
}

const exc_char_t* exception_base::get_message() const noexcept {
  return has_shared_data() ? as_shared_data()->str
                           : static_cast<const exc_char_t*>(m_data);
}

void* exception_base::create_masked_shared_data(const exc_char_t* msg,
                                                size_t msg_length) noexcept {
  auto* shared_data{create_shared_data(msg, msg_length)};
  auto data_as_num{reinterpret_cast<size_t>(shared_data)};
  data_as_num |= SHARED_DATA_MASK;
  return reinterpret_cast<void*>(data_as_num);
}

exception_data* exception_base::create_shared_data(const exc_char_t* msg,
                                                   size_t msg_length) noexcept {
  auto* buffer{static_cast<byte*>(alloc_non_paged(
      sizeof(exception_data) +
      (msg_length + 1) * sizeof(exc_char_t)))};  // Длина с учётом нуль-символа
  crt_critical_failure_if_not(buffer);  // terminate if allocation fails

  auto* msg_buf{reinterpret_cast<exc_char_t*>(buffer + sizeof(exception_data))};
  char_traits<exc_char_t>::copy(msg_buf, msg, msg_length);
  msg_buf[msg_length] = static_cast<exc_char_t>(0);

  return new (buffer) exception_data{msg_buf, 1};
}

void exception_base::destroy_shared_data(exception_data* target) noexcept {
  ktl::free(target);
}
}  // namespace ktl::crt