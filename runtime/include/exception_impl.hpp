#pragma once
#include <exception_base.h>

namespace ktl {
class exception : public crt::exception_base {
 public:
  using MyBase = crt::exception_base;

 public:
  using MyBase::MyBase;
  virtual const exc_char_t* what() const noexcept { return get_message(); }
};

class bad_alloc : public exception {
 public:
  using MyBase = exception;

 public:
  bad_alloc() noexcept
      : MyBase{L"memory allocation fails", constexpr_message_tag{}} {}

 protected:
  using MyBase::MyBase;
};

class bad_array_length : public bad_alloc {
 public:
  using MyBase = bad_alloc;

 public:
  bad_array_length() noexcept
      : MyBase{L"bad array length", constexpr_message_tag{}} {}
};
}  // namespace ktl