#pragma once
#include <exception_base.hpp>

#include <ntddk.h>

namespace ktl {
class exception : public crt::exception_base {
 public:
  using MyBase = crt::exception_base;

 public:
  using MyBase::MyBase;
  virtual const exc_char_t* what() const noexcept;
  virtual NTSTATUS code() const noexcept;
};

class bad_alloc : public exception {
 public:
  using MyBase = exception;

 public:
  constexpr bad_alloc() noexcept
      : MyBase{L"memory allocation fails", persistent_message_tag{}} {}

  NTSTATUS code() const noexcept override;

 protected:
  using MyBase::MyBase;
};

class bad_array_length : public bad_alloc {
 public:
  using MyBase = bad_alloc;

 public:
  constexpr bad_array_length() noexcept;
  NTSTATUS code() const noexcept override;
};
}  // namespace ktl