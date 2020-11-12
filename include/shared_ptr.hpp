#pragma once
#include <heap.h>
#include <type_traits.hpp>
#include <utility.hpp>

#ifndef NO_CXX_STANDARD_LIBRARY
namespace winapi::kernel::mm {
#include <memory>
using std::shared_ptr;
}  // namespace winapi::kernel::mm
#else
namespace winapi::kernel::mm {
namespace details {
struct external_pointer_tag {};
struct jointly_allocated_tag {};

class ref_counter_base {
 protected:
  virtual void destory_object() = 0;
  virtual void delete_this() = 0;

 private:
};

template <class Ty>
class ref_counter_ptr_holder : public ref_counter_base {
 public:
  ref_counter_ptr_holder(const ref_counter_ptr_holder&) = delete;
  ref_counter_ptr_holder& operator=(const ref_counter_ptr_holder&) = delete;

  ref_counter_ptr_holder(Ty* ptr) noexcept : m_ptr{ptr} {}

 protected:
  Ty* get_ptr() { return m_ptr; }

 private:
  Ty* m_ptr;
};

template <class Ty, class Deletertag>
class ref_counter : public ref_counter_ptr_holder<Ty>;

template <class Ty>
class ref_counter<Ty, external_pointer_tag>
    : public ref_counter_ptr_holder<Ty> {
 public:
  using MyBase = ref_counter_ptr_holder<Ty>;

 protected:
  void destory_object() override {
    Ty* target{MyBase::get_ptr()};
    delete target;
  }
  void delete_this() override { delete this; }
};

template <class Ty>
class ref_counter<Ty, jointly_allocated_tag>
    : public ref_counter_ptr_holder<Ty> {
 public:
  using MyBase = ref_counter_ptr_holder<Ty>;

 protected:
  void destory_object() override {
    Ty* target{MyBase::get_ptr()};
    target->~Ty();
  }
  void delete_this() override { delete this; }
};
}  // namespace details
}  // namespace winapi::kernel::mm
#endif