#pragma once
#include <exception_impl.hpp>

namespace ktl {
class logic_error : public exception {
 public:
  using MyBase = exception;

 public:
  using MyBase::MyBase;
};

class out_of_range: public logic_error {
 public:
  using MyBase = logic_error;

 public:
  using MyBase::MyBase;
};

class runtime_error : public exception {
 public:
  using MyBase = exception;

 public:
  using MyBase::MyBase;
};

class overflow_error : public runtime_error {
 public:
  using MyBase = runtime_error;

 public:
  using MyBase::MyBase;
};
}  // namespace ktl