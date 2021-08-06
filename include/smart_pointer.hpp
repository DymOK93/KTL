#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <memory>
namespace ktl {
using std::shared_ptr;
using std::unique_ptr;
using std::weak_ptr;
}  // namespace ktl
#else
#include <exception.h>
#include <heap.h>
#include <allocator.hpp>
#include <atomic.hpp>
#include <compressed_pair.hpp>
#include <functional.hpp>
#include <memory_type_traits.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

namespace ktl {
namespace mm::details {
template <class Ty>
struct default_delete {
  using enable_delete_null =
      true_type;  //Не требует проверки на nullptr перед вызовом

  constexpr default_delete() = default;

  template <class OtherTy,
            enable_if_t<is_convertible_v<OtherTy*, Ty*>, int> = 0>
  default_delete(const default_delete<OtherTy>&) noexcept {}

  void operator()(Ty* ptr) const noexcept {
    static_assert(sizeof(Ty) > 0, "Can't delete an incomplete type");
    delete ptr;
  }
};
template <class Ty>
struct default_delete<Ty[]> {
  using enable_delete_null =
      true_type;  //Не требует проверки на nullptr перед вызовом

  constexpr default_delete() = default;

  template <class OtherTy,
            enable_if_t<is_convertible_v<OtherTy*, Ty*>, int> = 0>
  default_delete(const default_delete<OtherTy>&) noexcept {}

  void operator()(Ty* ptr) const noexcept {
    static_assert(sizeof(Ty) > 0, "Can't delete an incomplete type");
    delete ptr;
  }
};
}  // namespace mm::details

template <class Ty, class Deleter = mm::details::default_delete<Ty> >
class unique_ptr {
 public:
  using pointer = mm::details::get_pointer_type_t<Deleter, Ty>;
  using element_type = Ty;
  using deleter_type = Deleter;

 private:
  template <class,
            class>  //Для конструирования из unique_ptr с указателем
                    //другого типа
  friend class unique_ptr;  //(например, для
                            //полиморфных объектов)

 public:
  template <class Dx = Deleter,
            enable_if_t<!is_pointer_v<Dx> &&  //Нельзя хранить nullptr
                                              //в качестве deleter'a
                            is_default_constructible_v<Dx>,
                        int> = 0>
  constexpr unique_ptr() noexcept {}

  constexpr unique_ptr(nullptr_t) noexcept {}

  explicit unique_ptr(pointer ptr) noexcept
      : m_value{zero_then_variadic_args{}, ptr} {}

  template <class Dx = Deleter,
            enable_if_t<is_constructible_v<Dx, const Dx&>, int> = 0>
  unique_ptr(pointer ptr, const Deleter& deleter) noexcept(
      is_nothrow_copy_constructible_v<Dx>)
      : m_value{deleter, ptr} {}

  // Dx - не ссылочный тип
  template <
      class Dx = Deleter,
      enable_if_t<!is_reference_v<Dx> && is_move_constructible_v<Dx>, int> = 0>
  unique_ptr(pointer ptr,
             Deleter&& deleter) noexcept(is_nothrow_move_constructible_v<Dx>)
      : m_value{move(deleter), ptr} {}

  // Dx - ссылочный тип
  template <class Dx = Deleter,
            enable_if_t<is_reference_v<Dx> &&
                            is_constructible_v<Dx, remove_reference_t<Dx> >,
                        int> = 0>
  unique_ptr(pointer ptr, remove_reference_t<Dx>&& deleter) =
      delete;  //Нельзя сконструировать ссылочный тип Dx& с аргументом Dx&&
               //(r-value ref)

  template <class Dx = Deleter,
            enable_if_t<is_move_constructible_v<Dx>, int> = 0>
  unique_ptr(unique_ptr&& other) noexcept(is_nothrow_move_constructible_v<Dx>)
      : m_value{forward<deleter_type>(other.get_deleter()), other.release()} {}

  template <
      class OtherTy,
      class OtherDeleter,
      enable_if_t<
          !is_array_v<OtherTy> &&
              is_convertible_v<
                  typename unique_ptr<OtherTy, OtherDeleter>::pointer,
                  pointer> &&
              conditional_t<is_reference_v<OtherDeleter>,
                            is_same<OtherDeleter,
                                    deleter_type>,  //При хранении
                                                    //ссылки на deleter
                                                    //типы OtherDeleter
                                                    //и deleter_type
                                                    //должны совпадать
                            is_convertible<OtherDeleter, deleter_type> >::value,
          int> = 0>
  unique_ptr(unique_ptr<OtherTy, OtherDeleter>&& other) noexcept(
      is_nothrow_constructible_v<deleter_type, OtherDeleter>)
      : m_value{forward<deleter_type>(other.get_deleter()), other.release()} {}

  template <class Dx = Deleter, enable_if_t<is_move_assignable_v<Dx>, int> = 0>
  unique_ptr& operator=(unique_ptr&& other) noexcept(
      is_nothrow_move_assignable_v<Dx>) {
    if (addressof(other) != this) {
      reset(other.release());
      get_deleter() = forward<deleter_type>(other.get_deleter());
    }
    return *this;
  }

  template <
      class OtherTy,
      class OtherDeleter,
      enable_if_t<!is_array_v<OtherTy> &&
                      is_convertible_v<
                          typename unique_ptr<OtherTy, OtherDeleter>::pointer,
                          pointer> &&
                      is_assignable_v<deleter_type, OtherDeleter>,
                  int> = 0>
  unique_ptr& operator=(unique_ptr<OtherTy, OtherDeleter>&& other) noexcept(
      is_nothrow_assignable_v<deleter_type, OtherDeleter>) {
    //Проверка addressof(other) != this не обязательная: в таком случае
    //будет выбран предыдущий шаблон, а там проверка есть
    reset(other.release());
    get_deleter() = forward<deleter_type>(other.get_deleter());
    return *this;
  }

  unique_ptr(const unique_ptr&) = delete;
  unique_ptr& operator=(const unique_ptr&) = delete;

  ~unique_ptr() { reset(); }

  pointer release() noexcept { return exchange(get_ref_to_ptr(), nullptr); }

  void reset(pointer ptr = pointer{}) noexcept {
    pointer target{exchange(get_ref_to_ptr(), ptr)};
    if constexpr (mm::details::get_enable_delete_null_v<
                      deleter_type>) {  //Поддерживает
      //передачу nullptr в
      //качестве аргумента
      get_deleter()(target);
    } else {
      if (target) {
        get_deleter()(target);
      }
    }
  }

  void swap(unique_ptr& other) noexcept(is_nothrow_swappable_v<deleter_type>) {
    swap(m_value, other.m_value);
  }

  pointer get() const noexcept { return m_value.get_second(); }

  Deleter& get_deleter() noexcept { return m_value.get_first(); }
  const Deleter& get_deleter() const noexcept { return m_value.get_first(); }

  operator bool() const noexcept { return static_cast<bool>(get()); }

  add_lvalue_reference_t<Ty> operator*() const& noexcept { return *get(); }

  add_rvalue_reference_t<Ty> operator*() const&& noexcept {
    return move(*get());
  }

  pointer operator->() const noexcept { return m_value.get_second(); }

 private:
  pointer& get_ref_to_ptr() { return m_value.get_second(); }

 private:
  compressed_pair<deleter_type, pointer> m_value{};
};

template <class Ty1, class Dx1, class Ty2, class Dx2>
bool operator==(const unique_ptr<Ty1, Dx1>& lhs,
                const unique_ptr<Ty2, Dx2>& rhs) noexcept {
  return lhs.get() == rhs.get();
}

template <class Ty1, class Dx1, class Ty2, class Dx2>
bool operator!=(const unique_ptr<Ty1, Dx1>& lhs,
                const unique_ptr<Ty1, Dx2>& rhs) noexcept {
  return !(lhs == rhs);
}

template <class Ty1, class Dx1, class Ty2, class Dx2>
bool operator<(const unique_ptr<Ty1, Dx1>& lhs,
               const unique_ptr<Ty2, Dx2>& rhs) noexcept {
  using common_t = common_type_t<typename unique_ptr<Ty1, Dx1>::pointer,
                                 typename unique_ptr<Ty2, Dx2>::pointer>;
  return less<add_const<common_t> >{}(lhs.get(), rhs.get());
}

template <class Ty1, class Dx1, class Ty2, class Dx2>
bool operator>(const unique_ptr<Ty1, Dx1>& lhs,
               const unique_ptr<Ty2, Dx2>& rhs) noexcept {
  using common_t = common_type_t<typename unique_ptr<Ty1, Dx1>::pointer,
                                 typename unique_ptr<Ty2, Dx2>::pointer>;
  return greater<add_const<common_t> >{}(lhs.get(), rhs.get());
}

template <class Ty1, class Dx1, class Ty2, class Dx2>
bool operator<=(const unique_ptr<Ty1, Dx1>& lhs,
                const unique_ptr<Ty2, Dx2>& rhs) noexcept {
  return !(lhs > rhs);
}

template <class Ty1, class Dx1, class Ty2, class Dx2>
bool operator>=(const unique_ptr<Ty1, Dx1>& lhs,
                const unique_ptr<Ty2, Dx2>& rhs) noexcept {
  return !(lhs < rhs);
}

template <class Ty, class Dx>
bool operator==(const unique_ptr<Ty, Dx>& ptr, nullptr_t) noexcept {
  return !ptr;
}

template <class Ty, class Dx>
bool operator!=(const unique_ptr<Ty, Dx>& ptr, nullptr_t) noexcept {
  return ptr;
}

template <class Ty, class Dx>
bool operator==(nullptr_t, const unique_ptr<Ty, Dx>& ptr) noexcept {
  return !ptr;
}

template <class Ty, class Dx>
bool operator!=(nullptr_t, const unique_ptr<Ty, Dx>& ptr) noexcept {
  return ptr;
}

template <class Ty, class Dx>
bool operator<(const unique_ptr<Ty, Dx>& ptr, nullptr_t null) noexcept {
  return less<add_const<Ty> >(ptr.get(), null);
}

template <class Ty, class Dx>
bool operator>(const unique_ptr<Ty, Dx>& ptr, nullptr_t null) noexcept {
  return greater<add_const<Ty> >(ptr.get(), null);
}

template <class Ty, class Dx>
bool operator<(nullptr_t null, const unique_ptr<Ty, Dx>& ptr) noexcept {
  return less<add_const<Ty> >(null, ptr.get());
}

template <class Ty, class Dx>
bool operator>(nullptr_t null, const unique_ptr<Ty, Dx>& ptr) noexcept {
  return greater<add_const<Ty> >(null, ptr.get());
}

template <class Ty, class Dx>
bool operator<=(const unique_ptr<Ty, Dx>& ptr, nullptr_t null) noexcept {
  return !(ptr > null);
}

template <class Ty, class Dx>
bool operator<=(nullptr_t null, const unique_ptr<Ty, Dx>& ptr) noexcept {
  return !(null > ptr);
}

template <class Ty, class Dx>
bool operator>=(const unique_ptr<Ty, Dx>& ptr, nullptr_t null) noexcept {
  return !(ptr < null);
}

template <class Ty, class Dx>
bool operator>=(nullptr_t null, const unique_ptr<Ty, Dx>& ptr) noexcept {
  return !(null < ptr);
}

// C++17 deduction guide
template <class Ty>
unique_ptr(Ty*) -> unique_ptr<Ty>;

// C++17 deduction guide
template <class Ty, class Dx>
unique_ptr(Ty*, Dx&&) -> unique_ptr<Ty, Dx>;

/********************************************************************************
В соответствии с правилами вывода типов шаблонных аргументов
deduction guide выше может привести к неочевидным последствиям:

auto make_resource_guard(some_type* resource) {
auto guard = [](some_type* res){ release_resource(res); };
using guard_type = decltype(guard);
return unique_ptr(resource, guard);
}

Тип возвращаемого значения окажется
unique_ptr<some_type, guard_type&>!
Во избежание получения висячей ссылки
ниже добавлено дополнительно правило, запрещающее автоматическую подстановку
l-value ref в аргумент шаблона.
При необходимости сконструировать unique_ptr, хранящий ссылку на deleter,
необходимо указать это явно:
unique_ptr<some_type, deleter_type&> uptr(...);
*********************************************************************************/

// C++17 deduction guide
template <class Ty, class Dx>
unique_ptr(Ty*, Dx&) -> unique_ptr<Ty, Dx>;

template <class Ty, class... Types>
unique_ptr<Ty> make_unique(Types&&... args) {
  return unique_ptr<Ty>(new Ty(forward<Types>(args)...));
}

namespace mm::details {
struct external_pointer_tag {};
struct jointly_allocated_tag {};

class ref_counter_base {
  using counter_t = uint32_t;

 public:
  constexpr ref_counter_base() noexcept = default;
  virtual ~ref_counter_base() noexcept = default;

  void Acquire() noexcept { inc_strong_refs(); }

  void Release() noexcept {
    if (dec_strong_refs() == 0) {
      destroy_object();
      Unfollow();
    }
  }

  void Follow() noexcept { inc_weak_refs(); }

  void Unfollow() noexcept {
    if (dec_weak_refs() == 0) {
      delete_this();
    }
  }

  size_t StrongRefsCount() const noexcept {
    return m_strong_counter.load<memory_order_relaxed>();
  }
  size_t WeakRefsCount() const noexcept {
    return m_weak_counter.load<memory_order_relaxed>() - 1u;
  }  // 1u is a default counter value

 protected:
  virtual void destroy_object() noexcept = 0;
  virtual void delete_this() noexcept = 0;

 private:
  counter_t inc_strong_refs() noexcept { return ++m_strong_counter; }
  counter_t inc_weak_refs() noexcept { return ++m_weak_counter; }
  counter_t dec_strong_refs() noexcept { return --m_strong_counter; }
  counter_t dec_weak_refs() noexcept { return --m_weak_counter; }

 private:
  atomic<counter_t> m_strong_counter{1};
  atomic<counter_t> m_weak_counter{1};
};

template <class First, class Second, class ConcreteRefCounter>
class ref_counter_with_storage : public ref_counter_base,
                                 public compressed_pair<First, Second> {
 public:
  using MyRefCounterBase = ref_counter_base;
  using MyStorageBase = compressed_pair<First, Second>;

 private:
  struct accessor : ConcreteRefCounter {
    static void destroy_object(ConcreteRefCounter& ref_counter) noexcept {
      auto fn_ptr{&ConcreteRefCounter::destroy_object_impl};
      (ref_counter.*fn_ptr)();
    }

    static void delete_this(ConcreteRefCounter& ref_counter) noexcept {
      auto fn_ptr{&ConcreteRefCounter::delete_this_impl};
      (ref_counter.*fn_ptr)();
    }
  };

 public:
  using MyStorageBase::MyStorageBase;

  using MyStorageBase::get_first;
  using MyStorageBase::get_second;

 private:
  void destroy_object() noexcept final {
    accessor::destroy_object(get_context());
  }

  void delete_this() noexcept final { accessor::delete_this(get_context()); }

  ConcreteRefCounter& get_context() noexcept {
    return static_cast<ConcreteRefCounter&>(*this);
  }
};

template <class Ty,
          class Deleter,
          class DestroyObjectPolicy,
          class DeleteItselfPolicy>
class ref_counter_with_deleter
    : public ref_counter_with_storage<
          Deleter,
          Ty*,
          ref_counter_with_deleter<Ty,
                                   Deleter,
                                   DestroyObjectPolicy,
                                   DeleteItselfPolicy> > {
 public:
  using MyBase =
      ref_counter_with_storage<Deleter,
                               Ty*,
                               ref_counter_with_deleter<Ty,
                                                        Deleter,
                                                        DestroyObjectPolicy,
                                                        DeleteItselfPolicy> >;
  using deleter_type = Deleter;

 public:
  constexpr ref_counter_with_deleter() = default;

  template <class Dx = Deleter,
            enable_if_t<is_default_constructible_v<Dx>, int> = 0>
  ref_counter_with_deleter(Ty* ptr) noexcept(
      is_nothrow_default_constructible_v<Dx>)
      : MyBase(zero_then_variadic_args{}, ptr) {}

  template <class Dx>
  ref_counter_with_deleter(Ty* ptr, Dx&& deleter) noexcept(
      is_nothrow_constructible_v<Deleter, Dx>)
      : MyBase(one_then_variadic_args{}, forward<Dx>(deleter), ptr) {}

 protected:
  void destroy_object_impl() noexcept {
    DestroyObjectPolicy::Apply(MyBase::get_second(), MyBase::get_first());
  }

  void delete_this_impl() noexcept {
    DeleteItselfPolicy::Apply<Ty>(this, MyBase::get_first());
  }
};

template <class Ty,
          class Deleter,
          class Alloc,
          class DestroyObjectPolicy,
          class DeleteItselfPolicy>
class ref_counter_with_deleter_and_alloc
    : public ref_counter_with_storage<
          Alloc,
          compressed_pair<Deleter, Ty*>,
          ref_counter_with_deleter_and_alloc<Ty,
                                             Deleter,
                                             Alloc,
                                             DestroyObjectPolicy,
                                             DeleteItselfPolicy> > {
 public:
  using MyBase = ref_counter_with_storage<
      Alloc,
      compressed_pair<Deleter, Ty*>,
      ref_counter_with_deleter_and_alloc<Ty,
                                         Deleter,
                                         Alloc,
                                         DestroyObjectPolicy,
                                         DeleteItselfPolicy> >;
  using deleter_type = Deleter;
  using allocator_type = Alloc;

 private:
  using storage_type = compressed_pair<Deleter, Ty*>;

 public:
  constexpr ref_counter_with_deleter_and_alloc() = default;

  template <class Dx = Deleter,
            class Alc = Alloc,
            enable_if_t<is_default_constructible_v<Dx> &&
                            is_default_constructible_v<Alc>,
                        int> = 0>
  ref_counter_with_deleter_and_alloc(Ty* ptr) noexcept(
      is_nothrow_default_constructible_v<Dx>&&
          is_nothrow_default_constructible_v<Alc>)
      : MyBase(zero_then_variadic_args{},
               storage_type{zero_then_variadic_args{}, ptr}) {}

  template <class Dx, class Alc>
  ref_counter_with_deleter_and_alloc(
      Ty* ptr,
      Dx&& deleter,
      Alc&& alloc) noexcept(is_nothrow_constructible_v<Deleter, Dx>&&
                                is_nothrow_constructible_v<Alloc, Alc>)
      : MyBase(
            forward<Alc>(alloc),
            storage_type{one_then_variadic_args{}, forward<Dx>(deleter), ptr}) {
  }

  // const deleter_type& get_deleter() const noexcept {
  //  return this->m_storage.get_first().get_first();
  //}

  // deleter_type& get_deleter() noexcept {
  //  return this->m_storage.get_first().get_first();
  //}

  // const allocator_type& get_allocator() const noexcept {
  //  return this->m_storage.get_first().get_second();
  //}

  // allocator_type& get_allocator() noexcept {
  //  return this->m_storage.get_first().get_second();
  //}

 protected:
  void destroy_object_impl() noexcept {
    DestroyObjectPolicy::Apply(MyBase::get_second().get_second(),
                               MyBase::get_second().get_first());
  }

  void delete_this_impl() noexcept {
    DeleteItselfPolicy::Apply<Ty>(MyBase::get_second().get_second(),
                                  MyBase::get_first());
  }
};

struct DestroyObjectWithDeleter {
  template <class Ty, class Deleter>
  static void Apply(Ty* target, Deleter& deleter) {
    if constexpr (get_enable_delete_null_v<Deleter>) {
      deleter(target);
    } else {
      if (target) {
        deleter(target);
      }
    }
  }
};

struct DestroyObjectWithAllocator {
  template <class Ty, class Alloc>
  static void Apply(Ty* target, Alloc& alloc) {
    allocator_traits<Alloc>::destroy(alloc, target);
  }
};

struct DeleteItself {
  template <class Ty, class RefCounter, class Deleter>
  static void Apply(RefCounter* ref_counter,
                    [[maybe_unused]] Deleter& deleter) {
    delete ref_counter;
  }
};

struct DeallocateItself {
  template <class Ty, class RefCounter, class Alloc>
  static void Apply(RefCounter* ref_counter, Alloc& allocator) {
    auto alloc{move(allocator)};
    destroy_at(ref_counter);
    allocator_traits<Alloc>::deallocate_bytes(alloc, ref_counter,
                                              sizeof(RefCounter));
  }
};

struct DestroyObjectAndItself {
  template <class Ty, class RefCounter, class Alloc>
  static void Apply(RefCounter* ref_counter, Alloc& allocator) {
    auto alloc{move(allocator)};
    destroy_at(ref_counter);
    allocator_traits<Alloc>::deallocate_bytes(alloc, ref_counter,
                                              sizeof(pair<RefCounter, Ty>));
  }
};

template <class Ty, class ConcretePtr>  // CRTP
class refcounted_ptr_base {
 public:
  using ref_counter_base = ref_counter_base;
  using element_type = remove_extent_t<Ty>;

 public:
  refcounted_ptr_base(const refcounted_ptr_base&) = delete;
  refcounted_ptr_base& operator=(const refcounted_ptr_base&) = delete;

  size_t use_count() const noexcept {
    return m_ref_counter ? m_ref_counter->StrongRefsCount() : 0;
  }

 protected:
  template <class U, class OtherPtr>
  friend class refcounted_ptr_base;

  constexpr refcounted_ptr_base() = default;

  explicit constexpr refcounted_ptr_base(element_type* value_ptr,
                                         ref_counter_base* ref_counter) noexcept
      : m_value_ptr{value_ptr}, m_ref_counter{ref_counter} {}

  constexpr Ty* get_value_ptr() const noexcept { return m_value_ptr; }

  constexpr ref_counter_base* get_ref_counter() const noexcept {
    return m_ref_counter;
  }

  void incref() noexcept;  // ConcretePtr must override there methods
  void decref() noexcept;

  template <class U>
  ConcretePtr& construct_from(U* ptr, ref_counter_base* ref_counter) noexcept {
    m_value_ptr = static_cast<Ty*>(ptr);
    m_ref_counter = ref_counter;
    return get_context();
  }

  template <class U, class OtherPtr>
  ConcretePtr& copy_construct_from(
      const refcounted_ptr_base<U, OtherPtr>& other) noexcept {
    static_assert(is_convertible_v<U*, Ty*>, "can't convert value pointer");
    decref_if_not_null();
    m_value_ptr = static_cast<Ty*>(other.m_value_ptr);
    m_ref_counter = other.m_ref_counter;
    incref_if_not_null();
    return get_context();
  }

  template <class U, class OtherPtr>
  ConcretePtr& move_construct_from(
      refcounted_ptr_base<U, OtherPtr>&& other) noexcept {
    static_assert(is_convertible_v<U*, Ty*>, "can't convert value pointer");
    decref_if_not_null();
    m_value_ptr = static_cast<Ty*>(exchange(other.m_value_ptr, nullptr));
    m_ref_counter = exchange(other.m_ref_counter, nullptr);
    return get_context();
  }

  void reset() {
    decref_if_not_null();
    m_value_ptr = nullptr;
    m_ref_counter = nullptr;
  }

  void swap(refcounted_ptr_base& other) noexcept {
    ktl::swap(m_value_ptr, other.m_value_ptr);
    ktl::swap(m_ref_counter, other.m_ref_counter);
  }

 private:
  void incref_if_not_null() noexcept {
    if (m_ref_counter) {
      get_context().incref();
    }
  }
  void decref_if_not_null() noexcept {
    if (m_ref_counter) {
      get_context().decref();
    }
  }

  ConcretePtr& get_context() noexcept {
    return static_cast<ConcretePtr&>(*this);
  }

 private:
  element_type* m_value_ptr{nullptr};
  ref_counter_base* m_ref_counter{nullptr};
};

}  // namespace mm::details

template <class Ty>
class shared_ptr;

template <class Ty>
class weak_ptr : public mm::details::refcounted_ptr_base<Ty, weak_ptr<Ty> > {
 public:
  using MyBase = mm::details::refcounted_ptr_base<Ty, weak_ptr<Ty> >;
  using typename MyBase::element_type;

 public:
  constexpr weak_ptr() noexcept = default;

  weak_ptr(const weak_ptr& other) noexcept {
    MyBase::copy_construct_from(other);
  }

  template <class OtherTy,
            enable_if_t<is_convertible_v<Ty*, OtherTy*>, int> = 0>
  weak_ptr(const weak_ptr<OtherTy>& other) noexcept {
    MyBase::copy_construct_from(other);
  }

  weak_ptr(const shared_ptr<Ty>& sptr) noexcept;  //Определены после shared_ptr

  template <class OtherTy,
            enable_if_t<is_convertible_v<Ty*, OtherTy*>, int> = 0>
  weak_ptr(const shared_ptr<OtherTy>& sptr) noexcept;

  weak_ptr(weak_ptr&& other) noexcept {
    MyBase::move_construct_from(move(other));
  }

  template <class OtherTy,
            enable_if_t<is_convertible_v<Ty*, OtherTy*>, int> = 0>
  weak_ptr(weak_ptr<OtherTy>&& other) noexcept {
    MyBase::move_construct_from(move(other));
  }

  weak_ptr& operator=(const weak_ptr& other) noexcept {
    if (addressof(other) != this) {
      MyBase::copy_construct_from(other);
    }
    return *this;
  }

  template <class OtherTy,
            enable_if_t<is_convertible_v<OtherTy*, Ty*>, int> = 0>
  weak_ptr& operator=(const weak_ptr<OtherTy>& other) noexcept {
    return MyBase::copy_construct_from(other);
  }

  weak_ptr& operator=(weak_ptr&& other) noexcept {
    if (addressof(other) != this) {
      MyBase::move_construct_from(move(other));
    }
    return *this;
  }

  template <class OtherTy,
            enable_if_t<is_convertible_v<OtherTy*, Ty*>, int> = 0>
  weak_ptr& operator=(weak_ptr<OtherTy>&& other) {
    return MyBase::move_construct_from(move(other));
  }

  ~weak_ptr() noexcept { reset(); }

  void reset() noexcept { MyBase::reset(); }

  void swap(weak_ptr& other) noexcept { MyBase::swap(other); }

  bool expired() const noexcept { return MyBase::use_count() == 0; }

  shared_ptr<Ty> lock() const noexcept;

 protected:
  friend class mm::details::refcounted_ptr_base<Ty, weak_ptr<Ty> >;  // MyBase
  void incref() noexcept { MyBase::get_ref_counter()->Follow(); }
  void decref() noexcept { MyBase::get_ref_counter()->Unfollow(); }
};

// Tag dispatch helpers
struct custom_deleter_tag_t {};
inline constexpr custom_deleter_tag_t custom_deleter_tag;
struct custom_allocator_tag_t {};
inline constexpr custom_allocator_tag_t custom_allocator_tag;

template <class Ty>
class shared_ptr
    : public mm::details::refcounted_ptr_base<Ty, shared_ptr<Ty> > {
 public:
  using MyBase = mm::details::refcounted_ptr_base<Ty, shared_ptr<Ty> >;
  using ref_counter_base = typename MyBase::ref_counter_base;
  using element_type = typename MyBase::element_type;
  using pointer = element_type*;
  using weak_type = weak_ptr<Ty>;

 public:
  constexpr shared_ptr() noexcept = default;

  constexpr shared_ptr(nullptr_t) noexcept {}

  explicit shared_ptr(Ty* ptr) noexcept
      : MyBase(ptr, make_default_ref_counter(ptr, is_array<Ty>{})) {}

  template <class U, enable_if_t<is_convertible_v<U*, Ty*>, int> = 0>
  shared_ptr(U* ptr) noexcept
      : MyBase(ptr, make_default_ref_counter(ptr, is_array<Ty>{})) {}

  template <class U, class Dx, enable_if_t<is_convertible_v<U*, Ty*>, int> = 0>
  shared_ptr(U* ptr, Dx&& deleter, custom_deleter_tag_t)
      : MyBase(ptr,
               make_ref_counter(
                   unique_ptr{ptr, [&deleter](U* target) { deleter(target); }},
                   ptr,
                   forward<Dx>(deleter))) {}

  template <class U,
            class Alloc,
            enable_if_t<is_convertible_v<U*, Ty*>, int> = 0>
  shared_ptr(U* ptr, Alloc&& alloc, custom_allocator_tag_t)
      : MyBase(ptr,
               allocate_default_ref_counter(ptr,
                                            forward<Alloc>(alloc),
                                            is_array<Ty>{})) {}

  // template <class Dx>
  // shared_ptr(nullptr_t, Dx&& deleter, custom_deleter_tag_t) {}

  // template <class Alloc>
  // shared_ptr(nullptr_t, Alloc&& alloc, custom_allocator_tag_t) {}

  // template <class OtherTy>
  // shared_ptr(const shared_ptr<OtherTy>& other, element_type* ptr) noexcept
  // {}

  // template <class OtherTy>
  // shared_ptr(shared_ptr<OtherTy>&& other, element_type* ptr) noexcept {}

  shared_ptr(const shared_ptr& other) noexcept {
    MyBase::copy_construct_from(other);
  }

  template <class OtherTy,
            enable_if_t<is_convertible_v<OtherTy*, Ty*>, int> = 0>
  shared_ptr(const shared_ptr<OtherTy>& other) noexcept {
    MyBase::copy_construct_from(other);
  }

  shared_ptr(shared_ptr&& other) noexcept {
    MyBase::move_construct_from(move(other));
  }

  template <class OtherTy,
            enable_if_t<is_convertible_v<OtherTy*, Ty*>, int> = 0>
  shared_ptr(shared_ptr<OtherTy>&& other) noexcept {
    MyBase::move_construct_from(move(other));
  }

  template <class U, enable_if_t<is_convertible_v<U*, Ty*>, int> = 0>
  shared_ptr(const weak_ptr<U>& wptr) {
    throw_exception_if<bad_weak_ptr>(wptr.expired());
    construct_from_weak(wptr);
  }

  template <class U, class Dx, enable_if_t<is_convertible_v<U*, Ty*>, int> = 0>
  shared_ptr(unique_ptr<U, Dx>&& uptr) {
    auto* ptr{uptr.get()};
    auto new_ref_counter{
        make_ref_counter(unique_ptr{ptr, [&deleter = uptr.get_deleter()](
                                             U* target) { deleter(target); }},
                         ptr, move(uptr.get_deleter()))};
    uptr.release();
    MyBase::construct_from(ptr, new_ref_counter);
  }

  shared_ptr& operator=(const shared_ptr& other) noexcept {
    if (addressof(other) != this) {
      MyBase::copy_construct_from(other);
    }
    return *this;
  }

  template <class OtherTy,
            enable_if_t<is_convertible_v<OtherTy*, Ty*>, int> = 0>
  shared_ptr& operator=(const shared_ptr<OtherTy>& other) noexcept {
    return MyBase::copy_construct_from(other);
  }

  shared_ptr& operator=(shared_ptr&& other) noexcept {
    if (addressof(other) != this) {
      MyBase::move_construct_from(move(other));
    }
    return *this;
  }

  template <class OtherTy,
            enable_if_t<is_convertible_v<OtherTy*, Ty*>, int> = 0>
  shared_ptr& operator=(shared_ptr<OtherTy>&& other) noexcept {
    return MyBase::move_construct_from(move(other));
  }

  template <class U, class Dx>
  shared_ptr& operator=(unique_ptr<U, Dx>&& uptr) {
    return *this = shared_ptr(move(uptr));
  }

  ~shared_ptr() noexcept { reset(); }

  void reset() noexcept { MyBase::reset(); }

  template <class U>
  void reset(U* ptr) noexcept {
    shared_ptr(ptr).swap(*this);
  }

  template <class U, class Dx>
  void reset(U* ptr, Dx&& deleter) noexcept {
    shared_ptr(ptr, forward<Dx>(deleter)).swap(*this);
  }

  void swap(shared_ptr& other) noexcept { MyBase::swap(other); }

  constexpr pointer get() const noexcept { return MyBase::get_value_ptr(); }

  constexpr add_lvalue_reference_t<element_type> operator*() const& {
    return *MyBase::get_value_ptr();
  }

  constexpr add_rvalue_reference_t<element_type> operator*() const&& {
    return move(*MyBase::get_value_ptr());
  }

  constexpr pointer operator->() const noexcept {
    return MyBase::get_value_ptr();
  }

  size_t use_count() const noexcept { return MyBase::use_count(); }
  constexpr operator bool() const noexcept { return get() != nullptr; }

 protected:
  template <class ValTy, class... Types>
  friend shared_ptr<ValTy> make_shared(Types&&... args);

  template <class ValTy, class Alloc, class... Types>
  friend shared_ptr<ValTy> allocate_shared(Alloc&& alloc, Types&&... args);

  friend class mm::details::refcounted_ptr_base<Ty, shared_ptr<Ty> >;
  void incref() noexcept { MyBase::get_ref_counter()->Acquire(); }
  void decref() noexcept { MyBase::get_ref_counter()->Release(); }

  shared_ptr(Ty* ptr, ref_counter_base* ref_counter) noexcept
      : MyBase(ptr, ref_counter) {}

 protected:
  template <class U, enable_if_t<is_convertible_v<U*, Ty*>, int> = 0>
  void construct_from_weak(const weak_ptr<U>& wptr) noexcept {
    MyBase::copy_construct_from(wptr);
  }

 private:
  template <class U>
  static ref_counter_base* make_default_ref_counter(U* ptr, true_type) {
    return make_ref_counter(unique_ptr<U[]>{}, ptr,
                            mm::details::default_delete<U[]>{});
  }

  template <class U>
  static ref_counter_base* make_default_ref_counter(U* ptr, false_type) {
    return make_ref_counter(unique_ptr<U>{}, ptr,
                            mm::details::default_delete<U>{});
  }

  template <class Guard, class U, class Deleter>
  static ref_counter_base* make_ref_counter(Guard&& temporary_guard,
                                            U* ptr,
                                            Deleter&& dx) {
    using namespace mm::details;

    ref_counter_base* ref_counter{nullptr};
    if (ptr) {
      temporary_guard.reset(ptr);
      ref_counter =
          new ref_counter_with_deleter<U, remove_reference_t<Deleter>,
                                       DestroyObjectWithDeleter, DeleteItself>(
              ptr, forward<Deleter>(dx));
      temporary_guard.release();
    }
    return ref_counter;
  }

  template <class U, class Alloc>
  static ref_counter_base* allocate_default_ref_counter(U* ptr,
                                                        Alloc&& alloc,
                                                        true_type) {
    return allocate_ref_counter_impl(unique_ptr<U[]>{}, ptr,
                                     forward<Alloc>(alloc),
                                     mm::details::default_delete<U[]>{});
  }

  template <class U, class Alloc>
  static ref_counter_base* allocate_default_ref_counter(U* ptr,
                                                        Alloc&& alloc,
                                                        false_type) {
    return allocate_ref_counter_impl(unique_ptr<U>{}, ptr,
                                     forward<Alloc>(alloc),
                                     mm::details::default_delete<U>{});
  }

  template <class Guard, class U, class Alloc, class Deleter>
  static ref_counter_base* allocate_ref_counter_impl(Guard&& value_guard,
                                                     U* ptr,
                                                     Alloc&& alloc,
                                                     Deleter&& dx) {
    using allocator_traits_type = allocator_traits<Alloc>;
    using size_type = allocator_traits_type::size_type;

    using ref_counter_t = mm::details::ref_counter_with_deleter_and_alloc<
        U, remove_reference_t<Deleter>, remove_reference_t<Alloc>,
        mm::details::DestroyObjectWithDeleter, mm::details::DeallocateItself>;
    constexpr auto REF_COUNTER_SIZE{
        static_cast<size_type>(sizeof(ref_counter_t))};

    value_guard.reset(ptr);
    auto* ref_counter{reinterpret_cast<ref_counter_t*>(
        allocator_traits_type::allocate_bytes(alloc, REF_COUNTER_SIZE))};
    unique_ptr ref_counter_guard{
        ref_counter, [&alloc, REF_COUNTER_SIZE](ref_counter_t* target) {
          allocator_traits_type::deallocate_bytes(alloc, target,
                                                  REF_COUNTER_SIZE);
        }};
    construct_at(ref_counter, ptr, forward<Deleter>(dx), forward<Alloc>(alloc));
    ref_counter_guard.release();
    value_guard.release();

    return ref_counter;
  }
};

template <class Ty>
weak_ptr<Ty>::weak_ptr(const shared_ptr<Ty>& sptr) noexcept {
  MyBase::copy_construct_from(sptr);
}

template <class Ty>
template <class OtherTy, enable_if_t<is_convertible_v<Ty*, OtherTy*>, int> >
weak_ptr<Ty>::weak_ptr(const shared_ptr<OtherTy>& sptr) noexcept {
  MyBase::copy_construct_from(sptr);
}

namespace mm::details {
template <class Ty>
struct shared_from_weak : shared_ptr<Ty> {
  using MyBase = shared_ptr<Ty>;

  template <class U, enable_if_t<is_convertible_v<U*, Ty*>, int> = 0>
  shared_ptr<Ty> operator()(const weak_ptr<U>& wptr) noexcept {
    if (!wptr.expired()) {
      MyBase::construct_from_weak(wptr);
    }
    return move(*this);
  }
};
}  // namespace mm::details

template <class Ty>
shared_ptr<Ty> weak_ptr<Ty>::lock() const noexcept {
  return mm::details::shared_from_weak<Ty>{}(*this);
}

template <class Ty1, class Ty2>
bool operator==(const shared_ptr<Ty1>& lhs,
                const shared_ptr<Ty2>& rhs) noexcept {
  return lhs.get() == rhs.get();
}

template <class Ty1, class Ty2>
bool operator!=(const shared_ptr<Ty1>& lhs,
                const shared_ptr<Ty2>& rhs) noexcept {
  return !(lhs == rhs);
}

template <class Ty1, class Ty2>
bool operator<(const shared_ptr<Ty1>& lhs,
               const shared_ptr<Ty2>& rhs) noexcept {
  using common_t = common_type_t<typename shared_ptr<Ty1>::pointer,
                                 typename shared_ptr<Ty2>::pointer>;
  return less<common_t>{}(lhs.get(), rhs.get());
}

template <class Ty1, class Ty2>
bool operator>(const shared_ptr<Ty1>& lhs,
               const shared_ptr<Ty2>& rhs) noexcept {
  using common_t = common_type_t<typename shared_ptr<Ty1>::pointer,
                                 typename shared_ptr<Ty2>::pointer>;
  return greater<common_t>{}(lhs.get(), rhs.get());
}

template <class Ty1, class Ty2>
bool operator<=(const shared_ptr<Ty1>& lhs,
                const shared_ptr<Ty2>& rhs) noexcept {
  return !(lhs > rhs);
}

template <class Ty1, class Ty2>
bool operator>=(const shared_ptr<Ty1>& lhs,
                const shared_ptr<Ty2>& rhs) noexcept {
  return !(lhs < rhs);
}

template <class Ty>
bool operator==(const shared_ptr<Ty>& ptr, nullptr_t) noexcept {
  return !ptr;
}

template <class Ty>
bool operator!=(const shared_ptr<Ty>& ptr, nullptr_t) noexcept {
  return ptr;
}

template <class Ty>
bool operator==(nullptr_t, const shared_ptr<Ty>& ptr) noexcept {
  return !ptr;
}

template <class Ty>
bool operator!=(nullptr_t, const shared_ptr<Ty>& ptr) noexcept {
  return ptr;
}

template <class Ty>
bool operator<(const shared_ptr<Ty>& ptr, nullptr_t null) noexcept {
  return less<add_const<Ty> >(ptr.get(), null);
}

template <class Ty>
bool operator>(const shared_ptr<Ty>& ptr, nullptr_t null) noexcept {
  return greater<add_const<Ty> >(ptr.get(), null);
}

template <class Ty>
bool operator<(nullptr_t null, const shared_ptr<Ty>& ptr) noexcept {
  return less<add_const<Ty> >(null, ptr.get());
}

template <class Ty>
bool operator>(nullptr_t null, const shared_ptr<Ty>& ptr) noexcept {
  return greater<add_const<Ty> >(null, ptr.get());
}

template <class Ty>
bool operator<=(const shared_ptr<Ty>& ptr, nullptr_t null) noexcept {
  return !(ptr > null);
}

template <class Ty>
bool operator<=(nullptr_t null, const shared_ptr<Ty>& ptr) noexcept {
  return !(null > ptr);
}

template <class Ty>
bool operator>=(const shared_ptr<Ty>& ptr, nullptr_t null) noexcept {
  return !(ptr < null);
}

template <class Ty>
bool operator>=(nullptr_t null, const shared_ptr<Ty>& ptr) noexcept {
  return !(null < ptr);
}

template <class Ty>
Ty* get_pointer(const shared_ptr<Ty>& ptr) noexcept {
  return ptr.get();
}

// C++17 deduction guides
template <class Ty>
shared_ptr(Ty*) -> shared_ptr<Ty>;

template <class Ty, class Dx>
shared_ptr(Ty*, Dx&&, custom_deleter_tag_t) -> shared_ptr<Ty>;

template <class Ty, class Alloc>
shared_ptr(Ty*, Alloc&&, custom_allocator_tag_t) -> shared_ptr<Ty>;

template <class Ty>
shared_ptr(weak_ptr<Ty>) -> shared_ptr<Ty>;

template <class Ty, class Dx>
shared_ptr(unique_ptr<Ty, Dx>) -> shared_ptr<Ty>;

template <class Ty, class... Types>
shared_ptr<Ty> make_shared(Types&&... args) {
  using ref_counter_base = mm::details::ref_counter_with_destroy_only<Ty>;
  constexpr size_t summary_size{sizeof(Ty) + sizeof(ref_counter_base)};
  auto* memory_block{operator new(summary_size)};
  Ty* object_ptr{reinterpret_cast<Ty*>(static_cast<byte*>(memory_block) +
                                       sizeof(ref_counter_base))};
  shared_ptr<Ty> sptr(object_ptr,
                      new (memory_block) ref_counter_base(
                          object_ptr));  //Конструирование ref_counter'a
  construct_at(object_ptr, forward<Types>(args)...);
  return sptr;
}

template <class Ty, class Alloc, class... Types>
shared_ptr<Ty> allocate_shared(Alloc&& alloc, Types&&... args) {
  using AlTy = remove_reference_t<Alloc>;
  static_assert(is_same_v<remove_reference_t<Ty>,
                          typename allocator_traits<
                              AlTy>::value_type>,  //Аллокатор должен работать
                                                   //с объектами типа Ty
                "Ty and Alloc::value_type must be same");

  using ref_counter_t =
      mm::details::ref_counter_with_allocator_deallocate_all<Ty, AlTy>;

  static constexpr size_t x = sizeof(
      mm::details::ref_counter_with_deleter_and_alloc<
          int, mm::details::default_delete<int>, basic_non_paged_allocator<int>,
          mm::details::DestroyObjectWithDeleter, mm::details::DeleteItself>);

  constexpr size_t summary_size{sizeof(Ty) + sizeof(ref_counter_base)};
  auto* memory_block{allocator_traits<AlTy>::allocate_bytes(
      alloc, summary_size)};  // Alloc должен предоставить allocate_bytes()
  Ty* object_ptr{reinterpret_cast<Ty*>(static_cast<byte*>(memory_block) +
                                       sizeof(ref_counter_base))};

  auto* ref_counter{construct_at(static_cast<ref_counter_base*>(memory_block),
                                 object_ptr, forward<Alloc>(alloc))};

  shared_ptr<Ty> sptr(object_ptr, ref_counter);
  allocator_traits<AlTy>::construct(ref_counter->get_alloc(), object_ptr,
                                    forward<Types>(args)...);

  return sptr;
}

template <class Ty, class Deleter>
void swap(unique_ptr<Ty, Deleter>& lhs,
          unique_ptr<Ty, Deleter>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
  lhs.swap(rhs);
}

template <class Ty>
void swap(shared_ptr<Ty>& lhs,
          shared_ptr<Ty>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
  lhs.swap(rhs);
}

template <class Ty>
void swap(weak_ptr<Ty>& lhs,
          weak_ptr<Ty>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
  lhs.swap(rhs);
}

template <class Ty>
Ty* get_pointer(const weak_ptr<Ty>& ptr) noexcept {
  return ptr.get();
}

namespace mm::details {
template <class Ty, class = void>
struct is_reference_countable : false_type {};

template <class Ty>
struct is_reference_countable<
    Ty,
    void_t<decltype(intrusive_ptr_add_ref(declval<add_pointer_t<Ty> >())),
           decltype(intrusive_ptr_release(declval<add_pointer_t<Ty> >()))> >
    : true_type {};

template <class Ty>
inline constexpr bool is_reference_countable_v =
    is_reference_countable<Ty>::value;
}  // namespace mm::details

template <class Ty>
class intrusive_ptr {
 public:
  using element_type = Ty;

  static_assert(mm::details::is_reference_countable_v<Ty>,
                "Ty must specialize intrusive_ptr_add_ref() and "
                "intrusive_ptr_release()");

 public:
  intrusive_ptr() noexcept = default;

  intrusive_ptr(Ty* ptr, bool add_ref = true) : m_ptr{ptr} {
    if (ptr && add_ref) {
      intrusive_ptr_add_ref(get());
    }
  }

  intrusive_ptr(const intrusive_ptr& other) : m_ptr{other.get()} {
    if (auto* ptr = get(); ptr) {
      intrusive_ptr_add_ref(ptr);
    }
  }

  intrusive_ptr(intrusive_ptr&& other) noexcept : m_ptr{other.detach()} {}

  template <class U, enable_if_t<is_convertible_v<U*, Ty*>, int> = 0>
  intrusive_ptr(const intrusive_ptr<U>& other)
      : m_ptr{static_cast<Ty*>(other.get())} {
    if (auto* ptr = get(); ptr) {
      intrusive_ptr_add_ref(ptr);
    }
  }

  template <class U, enable_if_t<is_convertible_v<U*, Ty*>, int> = 0>
  intrusive_ptr(intrusive_ptr<U>&& other) noexcept
      : m_ptr{static_cast<Ty*>(other.detach())} {}

  ~intrusive_ptr() noexcept {
    if (auto* ptr = get(); ptr) {
      intrusive_ptr_release(ptr);
    }
  }

  intrusive_ptr& operator=(const intrusive_ptr& other) {
    if (this != addressof(other)) {
      intrusive_ptr{other}.swap(*this);
    }
    return *this;
  }

  intrusive_ptr& operator=(intrusive_ptr&& other) noexcept {
    if (this != addressof(other)) {
      reset(other.detach(), false);
    }
    return *this;
  }

  template <class U>
  intrusive_ptr& operator=(const intrusive_ptr<U>& other) {
    intrusive_ptr{other}.swap(*this);
    return *this;
  }

  template <class U>
  intrusive_ptr& operator=(intrusive_ptr<U>&& other) noexcept {
    if (this != addressof(other)) {
      reset(static_cast<Ty*>(other.detach(), false));
    }
    return *this;
  }

  intrusive_ptr& operator=(Ty* ptr) {
    reset(ptr);
    return *this;
  }

  void reset(Ty* ptr = nullptr) { intrusive_ptr{ptr}.swap(*this); }
  void reset(Ty* ptr, bool add_ref) { intrusive_ptr{ptr, add_ref}.swap(*this); }

  Ty& operator*() const& noexcept { return *m_ptr; }
  Ty&& operator*() const&& noexcept { return move(*m_ptr); }
  Ty* operator->() const noexcept { return m_ptr; }
  Ty* get() const noexcept { return m_ptr; }

  Ty* detach() noexcept { return exchange(m_ptr, nullptr); }

  explicit operator bool() const noexcept { return m_ptr != nullptr; }

  void swap(intrusive_ptr& other) noexcept { ktl::swap(m_ptr, other.m_ptr); }

 private:
  Ty* m_ptr{nullptr};
};

template <class Ty, class U>
bool operator==(const intrusive_ptr<Ty>& lhs,
                const intrusive_ptr<U>& rhs) noexcept {
  return equal_to<const Ty*>(lhs.get(), rhs.get());
}

template <class Ty, class U>
bool operator!=(const intrusive_ptr<Ty>& lhs,
                const intrusive_ptr<U>& rhs) noexcept {
  return !(lhs == rhs);
}

template <class Ty>
bool operator==(const intrusive_ptr<Ty>& lhs, Ty* rhs) noexcept {
  return equal_to<const Ty*>(lhs.get(), rhs);
}

template <class Ty>
bool operator!=(const intrusive_ptr<Ty>& lhs, Ty* rhs) noexcept {
  return !(lhs == rhs);
}

template <class Ty>
bool operator==(Ty* lhs, const intrusive_ptr<Ty>& rhs) noexcept {
  return rhs == lhs;
}

template <class Ty>
bool operator!=(Ty* lhs, intrusive_ptr<Ty> const& rhs) noexcept {
  return rhs != lhs;
}

template <class Ty, class U>
bool operator<(const intrusive_ptr<Ty>& lhs,
               const intrusive_ptr<U>& rhs) noexcept {
  using common_ptr_t = common_type_t<const Ty*, const U*>;
  return less<common_ptr_t>(static_cast<common_ptr_t>(lhs.get()),
                            static_cast<common_ptr_t>(rhs.get()));
}

template <class Ty>
void swap(intrusive_ptr<Ty>& lhs, intrusive_ptr<Ty>& rhs) noexcept {
  lhs.swap(rhs);
}

template <class Ty>
Ty* get_pointer(const intrusive_ptr<Ty>& ptr) noexcept {
  return ptr.get();
}

namespace mm::details {
template <class Alloc, typename SizeTy>
struct alloc_temporary_guard_delete {
  using allocator_type = Alloc;
  using allocator_traits_type = allocator_traits<allocator_type>;
  using value_type = typename allocator_traits_type::value_type;
  using pointer = typename allocator_traits_type::pointer;

  alloc_temporary_guard_delete(Alloc& alc, SizeTy cnt)
      : alloc{addressof(alc)}, count{cnt} {}

  void operator()(pointer ptr) {
    allocator_traits_type::deallocate(*alloc, ptr, count);
  }

  Alloc* const alloc;
  SizeTy count;
};
}  // namespace mm::details

template <class Ty, class Alloc, typename SizeTy>
static constexpr auto make_alloc_temporary_guard(Ty* ptr,
                                                 Alloc& alc,
                                                 SizeTy count) {
  using deleter_type = mm::details::alloc_temporary_guard_delete<Alloc, SizeTy>;
  return unique_ptr<typename deleter_type::value_type, deleter_type>{
      static_cast<typename deleter_type::pointer>(ptr),
      deleter_type{alc, count}};
}
}  // namespace ktl
#endif
