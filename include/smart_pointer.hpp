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
      : m_value{/*zero_then_variadic_args{}*/ deleter_type{}, ptr} {}

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

  add_lvalue_reference_t<Ty> operator*()
      const& {  //Перегруженный Ty::operator*() способен вызвать
                //исключение
    return *get();
  }

  add_rvalue_reference_t<Ty> operator*() const&& { return move(*get()); }

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
 public:
  using atomic_counter_t = unsigned long;

 public:
  constexpr ref_counter_base() = default;
  virtual ~ref_counter_base() = default;

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
    return static_cast<size_t>(
        InterlockedAddRelease(  // std::shared_ptr здесь использует
                                // relaxed (NoFence), я не рискнул, т.к.
                                // не смог обосновать теоретически
            const_cast<volatile long*>(reinterpret_cast<const volatile long*>(
                addressof(m_strong_counter))),
            0));
  }
  size_t WeakRefsCount() const noexcept {
    auto count{InterlockedAddRelease(
        const_cast<volatile long*>(
            reinterpret_cast<const volatile long*>(addressof(m_weak_counter))),
        0)};
    return static_cast<size_t>(count) - 1;
  }  //Начальное значение счётчика - 1

 protected:
  virtual void destroy_object() noexcept = 0;  //Функции должны обеспечивать
                                               //гарантии отсутствия исключений
  virtual void delete_this() noexcept = 0;

 private:
  atomic_counter_t inc_strong_refs() noexcept { return ++m_strong_counter; }
  atomic_counter_t inc_weak_refs() noexcept { return ++m_weak_counter; }
  atomic_counter_t dec_strong_refs() noexcept { return --m_strong_counter; }
  atomic_counter_t dec_weak_refs() noexcept { return --m_weak_counter; }

 private:
  atomic_uint32_t m_strong_counter{1};
  atomic_uint32_t m_weak_counter{
      1};  //!< Для выполнения декремента без дополнительных проверок
};

template <class Ty>
class ref_counter_ptr_holder : public ref_counter_base {
 public:
  constexpr ref_counter_ptr_holder() = default;
  ref_counter_ptr_holder(Ty* ptr) noexcept : m_ptr{ptr} {}

  Ty* get_ptr() const noexcept { return m_ptr; }
  Ty* exchange_ptr(Ty* new_ptr) noexcept { return exchange(m_ptr, new_ptr); }

 protected:
  Ty* m_ptr{nullptr};
};

template <class Ty>
class ref_counter_with_default_delete_itself
    : public ref_counter_ptr_holder<Ty> {
 public:
  using MyBase = ref_counter_ptr_holder<Ty>;

 public:
  using MyBase::MyBase;

 protected:
  void delete_this() noexcept final { delete this; }
};

template <class Ty>
class ref_counter_with_scalar_delete
    : public ref_counter_with_default_delete_itself<Ty> {
 public:
  using MyBase = ref_counter_with_default_delete_itself<Ty>;

 public:
  using MyBase::MyBase;

 protected:
  void destroy_object() noexcept final { delete MyBase::exchange_ptr(nullptr); }
};

template <class Ty>
class ref_counter_with_array_delete
    : public ref_counter_with_default_delete_itself<Ty> {
 public:
  using MyBase = ref_counter_with_default_delete_itself<Ty>;

 public:
  using MyBase::MyBase;

 protected:
  void destroy_object() noexcept final {
    delete[] MyBase::exchange_ptr(nullptr);
  }
};

template <class Ty>
class ref_counter_with_destroy_only
    : public ref_counter_with_default_delete_itself<Ty> {
 public:
  using MyBase = ref_counter_with_default_delete_itself<Ty>;

 public:
  using MyBase::MyBase;

 protected:
  void destroy_object() noexcept final { MyBase::exchange_ptr(nullptr)->~Ty(); }
};

template <class Ty, class Deleter>
class ref_counter_with_custom_deleter
    : public ref_counter_with_default_delete_itself<Ty> {
 public:
  using MyBase = ref_counter_with_default_delete_itself<Ty>;

 public:
  template <class Dx>
  ref_counter_with_custom_deleter(Ty* ptr, Dx&& deleter) noexcept(
      is_nothrow_constructible_v<Deleter, Dx>)
      : MyBase(ptr), m_deleter(forward<Dx>(deleter)) {}

  Deleter& get_deleter() noexcept { return m_deleter; }
  const Deleter& get_deleter() const noexcept { return m_deleter; }

 protected:
  void destroy_object() noexcept final {
    Ty* target{MyBase::exchange_ptr(nullptr)};  //Лишняя Interlocked-операция?
    if constexpr (get_enable_delete_null_v<Deleter>) {
      get_deleter()(target);
    } else {
      if (target) {
        get_deleter()(target);
      }
    }
  }

 private:
  Deleter m_deleter;
};

template <class Alloc>
class allocator_holder_base {
 public:
  constexpr allocator_holder_base() = default;
  allocator_holder_base(const Alloc& alloc) : m_alloc(alloc) {}
  allocator_holder_base(Alloc&& alloc) : m_alloc(move(alloc)) {}

  Alloc& get_alloc() noexcept { return m_alloc; }
  const Alloc& get_alloc() const noexcept { return m_alloc; }

 private:
  Alloc m_alloc{};
};

template <class Ty, class Alloc>
class ref_counter_with_allocator_default_delete_itself
    : public ref_counter_with_default_delete_itself<Ty>,
      public allocator_holder_base<Alloc> {
 public:
  using MyRefCounterBase = ref_counter_with_default_delete_itself<Ty>;
  using MyAllocBase = allocator_holder_base<Alloc>;

 public:
  template <class Allocator>
  ref_counter_with_allocator_default_delete_itself(Ty* ptr, Allocator&& alloc)
      : MyRefCounterBase(ptr), MyAllocBase(forward<Allocator>(alloc)) {}

 protected:
  void destroy_object() noexcept final {
    auto& alloc = MyAllocBase::get_alloc();
    if (auto* target = MyRefCounterBase::exchange_ptr(nullptr); target) {
      allocator_traits<Alloc>::destroy(alloc, target);
      allocator_traits<Alloc>::deallocate(alloc, target, 1);
    }
  }
};

template <class Ty, class Alloc>
class ref_counter_with_allocator_deallocate_itself
    : public ref_counter_ptr_holder<Ty>,
      public allocator_holder_base<Alloc> {
 public:
  using MyRefCounterBase = ref_counter_ptr_holder<Ty>;
  using MyAllocBase = allocator_holder_base<Alloc>;

 public:
  template <class Allocator>
  ref_counter_with_allocator_deallocate_itself(Ty* ptr, Allocator&& alloc)
      : MyRefCounterBase(ptr), MyAllocBase(forward<Allocator>(alloc)) {}

 protected:
  void destroy_object() noexcept final {
    auto& alloc{MyAllocBase::get_alloc()};
    if (auto* target = MyRefCounterBase::exchange_ptr(nullptr); target) {
      allocator_traits<Alloc>::destroy(alloc, target);
    }
  }
  void delete_this() noexcept final {
    auto alloc{
        move(MyAllocBase::get_alloc())};  //После вызова деструктора обращаться
                                          //к аллокатору нельзя!
    destroy_at(this);
    allocator_traits<Alloc>::deallocate_bytes(alloc, this,
                                              sizeof(*this) + sizeof(Ty));
  }
};

template <class Ty, class ConcretePtr>  // CRTP
class PtrBase {
 public:
  using ref_counter_t = ref_counter_base;
  using element_type = Ty;

 public:
  PtrBase(const PtrBase&) = delete;
  PtrBase& operator=(const PtrBase&) = delete;

  size_t use_count() const noexcept {
    return m_ref_counter ? m_ref_counter->StrongRefsCount() : 0;
  }

 protected:
  template <class U, class OtherPtr>
  friend class PtrBase;

  constexpr PtrBase() = default;
  explicit PtrBase(Ty* ptr, ref_counter_t* ref_counter) noexcept
      : m_value_ptr{ptr}, m_ref_counter{ref_counter} {}

  Ty* get_value_ptr() const noexcept {
    return interlocked_compare_exchange_pointer<Ty>(addressof(m_value_ptr),
                                                    nullptr, nullptr);
  }
  ref_counter_t* get_ref_counter() const noexcept {
    return th::interlocked_compare_exchange_pointer<ref_counter_t>(
        addressof(m_ref_counter), nullptr, nullptr);
  }

  void incref() noexcept;  //Дочерние классы ConcretePtr
  void decref() noexcept;  //должны переопределить два этих метода

  template <class U, class OtherPtr>
  ConcretePtr& copy_construct_from(const PtrBase<U, OtherPtr>& other) noexcept {
    static_assert(is_convertible_v<U*, Ty*>, "Can't convert value pointer");
    decref_if_not_null();
    exchange_value_ptr(static_cast<Ty*>(other.m_value_ptr));
    exchange_ref_counter(other.m_ref_counter);
    incref_if_not_null();
    return get_context();
  }

  template <class U, class OtherPtr>
  ConcretePtr& move_construct_from(PtrBase<U, OtherPtr>&& other) noexcept {
    static_assert(is_convertible_v<U*, Ty*>, "Can't convert value pointer");
    decref_if_not_null();
    exchange_value_ptr(static_cast<Ty*>(other.exchange_value_ptr(nullptr)));
    exchange_ref_counter(other.exchange_ref_counter(nullptr));
    return get_context();
  }

  void reset() {
    decref_if_not_null();
    exchange_value_ptr(nullptr);
    exchange_ref_counter(nullptr);
  }

  void swap(PtrBase& other) noexcept {
    Ty* value_ptr{m_value_ptr};
    ref_counter_t* ref_counter{m_ref_counter};
    exchange_value_ptr(other.exchange_value_ptr(value_ptr));
    exchange_ref_counter(other.exchange_ref_counter(ref_counter));
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

  Ty* exchange_value_ptr(Ty* new_value_ptr) noexcept {
    return th::interlocked_exchange_pointer(addressof(m_value_ptr),
                                            new_value_ptr);
  }

  ref_counter_t* exchange_ref_counter(ref_counter_t* new_ref_counter) noexcept {
    return th::interlocked_exchange_pointer(addressof(m_ref_counter),
                                            new_ref_counter);
  }

 private:
  Ty* m_value_ptr{nullptr};
  ref_counter_t* m_ref_counter{nullptr};
};

}  // namespace mm::details

template <class Ty>
class shared_ptr;

template <class Ty>
class weak_ptr : public mm::details::PtrBase<Ty, weak_ptr<Ty> > {
 public:
  using MyBase = mm::details::PtrBase<Ty, weak_ptr<Ty> >;
  using typename MyBase::element_type;

 public:
  constexpr weak_ptr() = default;
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
  friend class mm::details::PtrBase<Ty, weak_ptr<Ty> >;  // MyBase
  void incref() noexcept { MyBase::get_ref_counter()->Follow(); }
  void decref() noexcept { MyBase::get_ref_counter()->Unfollow(); }
};

//Для выбора нужной перегрузки конструктора
struct custom_deleter_tag_t {};
inline constexpr custom_deleter_tag_t custom_deleter_tag;
struct custom_allocator_tag_t {};
inline constexpr custom_allocator_tag_t custom_allocator_tag;

template <class Ty>
class shared_ptr : public mm::details::PtrBase<Ty, shared_ptr<Ty> > {
 public:
  using MyBase = mm::details::PtrBase<Ty, shared_ptr<Ty> >;
  using ref_counter_t = typename MyBase::ref_counter_t;
  using element_type = Ty;
  using pointer = element_type*;
  using weak_type = weak_ptr<Ty>;

 public:
  constexpr shared_ptr() = default;
  constexpr shared_ptr(nullptr_t) noexcept {}

  explicit shared_ptr(Ty* ptr) noexcept
      : MyBase(ptr, make_ref_counter_and_share<false>(ptr)) {}

  template <class U, enable_if_t<is_convertible_v<U*, Ty*>, int> = 0>
  shared_ptr(U* ptr) noexcept
      : MyBase(ptr, make_ref_counter_and_share<is_array_v<U> >(ptr)) {}

  template <class U, class Dx, enable_if_t<is_convertible_v<U*, Ty*>, int> = 0>
  shared_ptr(U* ptr, Dx&& deleter, custom_deleter_tag_t)
      : MyBase(ptr,
               make_special_ref_counter<
                   mm::details::ref_counter_with_custom_deleter>(
                   ptr,
                   forward<Dx>(deleter))) {}

  template <class U,
            class Alloc,
            enable_if_t<is_convertible_v<U*, Ty*>, int> = 0>
  shared_ptr(U* ptr, Alloc&& alloc, custom_allocator_tag_t)
      : MyBase(ptr,
               make_special_ref_counter<
                   details::ref_counter_with_allocator_default_delete_itself>(
                   ptr,
                   forward<Alloc>(alloc))) {}

  template <class Dx>
  shared_ptr(nullptr_t, Dx&& deleter, custom_deleter_tag_t) {}

  template <class Alloc>
  shared_ptr(nullptr_t, Alloc&& alloc, custom_allocator_tag_t) {}

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
    if (wptr.expired()) {
      throw bad_weak_ptr{};
    }
    construct_from_weak(wptr);
  }

  template <class U, class Dx, enable_if_t<is_convertible_v<U*, Ty*>, int> = 0>
  shared_ptr(unique_ptr<U, Dx>&& uptr) {
    auto* ptr{uptr.get()};
    auto new_ref_counter{
        make_special_ref_counter<mm::details::ref_counter_with_custom_deleter>(
            ptr, move(uptr.get_deleter()))};
    uptr.release();  // при выбросе исключения в new указатель не будет утерян
    MyBase::move_construct_from(shared_ptr<U>(ptr, new_ref_counter));
  }

  shared_ptr& operator=(const shared_ptr& other) {
    if (addressof(other) != this) {
      MyBase::copy_construct_from(other);
    }
    return *this;
  }

  template <class OtherTy,
            enable_if_t<is_convertible_v<OtherTy*, Ty*>, int> = 0>
  shared_ptr& operator=(const shared_ptr<OtherTy>& other) {
    return MyBase::copy_construct_from(other);
  }

  shared_ptr& operator=(shared_ptr&& other) {
    if (addressof(other) != this) {
      MyBase::move_construct_from(move(other));
    }
    return *this;
  }

  template <class OtherTy,
            enable_if_t<is_convertible_v<OtherTy*, Ty*>, int> = 0>
  shared_ptr& operator=(shared_ptr<OtherTy>&& other) {
    MyBase::move_construct_from(move(other));
  }

  template <class U, class Dx>
  shared_ptr& operator=(unique_ptr<U, Dx>&& uptr) {
    auto* ptr{uptr.release()};
    MyBase::move_construct_from(MyBase(
        ptr,
        make_special_ref_counter<mm::details::ref_counter_with_custom_deleter>(
            ptr, move(uptr.get_deleter()))));
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

  pointer get() const noexcept { return MyBase::get_value_ptr(); }
  add_lvalue_reference<element_type> operator*() const& {
    return *MyBase::get_value_ptr();
  }
  add_rvalue_reference<element_type> operator*() const&& {
    return move(*MyBase::get_value_ptr());
  }
  pointer operator->() const noexcept { return MyBase::get_value_ptr(); }

  add_lvalue_reference<element_type> operator[](ptrdiff_t idx)
      const {  // UB, если хранится не массив или idx больше его длины!
    return *(MyBase::get_value_ptr() + idx);
  }

  size_t use_count() const noexcept { return MyBase::use_count(); }
  operator bool() const noexcept { return static_cast<bool>(get()); }

 protected:
  template <class ValTy, class... Types>  //Не Ty, чтобы не скрывать основной
                                          //шаблонный параметр класса
  friend shared_ptr<ValTy> make_shared(Types&&... args);

  template <class ValTy, class Alloc, class... Types>
  friend shared_ptr<ValTy> allocate_shared(Alloc&& alloc, Types&&... args);

  friend class mm::details::PtrBase<Ty, shared_ptr<Ty> >;
  void incref() noexcept { MyBase::get_ref_counter()->Acquire(); }
  void decref() noexcept { MyBase::get_ref_counter()->Release(); }

  shared_ptr(Ty* ptr, ref_counter_t* ref_counter) noexcept
      : MyBase(ptr, ref_counter) {}

 protected:
  template <class U, enable_if_t<is_convertible_v<U*, Ty*>, int> = 0>
  void construct_from_weak(const weak_ptr<U>& wptr) noexcept {
    MyBase::copy_construct_from(wptr);
  }

 private:
  template <bool is_array, class U>
  static ref_counter_t* make_ref_counter_and_share(U* ptr) noexcept {
    if (ptr) {
      if constexpr (is_array) {
        return new mm::details::ref_counter_with_array_delete<U>(ptr);
      } else {
        return new mm::details::ref_counter_with_scalar_delete<U>(ptr);
      }
    }
    return nullptr;
  }

  template <template <class, class> class RefCounter, class U, class Destroyer>
  static ref_counter_t*
  make_special_ref_counter(U* ptr, Destroyer&& destroyer) noexcept(
      is_nothrow_constructible_v<remove_reference_t<Destroyer>, Destroyer>) {
    return new RefCounter<U, remove_reference_t<Destroyer> >(
        ptr,
        forward<Destroyer>(
            destroyer));  //Конструирование с нулевым указателем
                          //и кастомным Deleter'ом/Allocator'ом допускается
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
  using ref_counter_t = mm::details::ref_counter_with_destroy_only<Ty>;
  constexpr size_t summary_size{sizeof(Ty) + sizeof(ref_counter_t)};
  auto* memory_block{operator new(summary_size)};
  Ty* object_ptr{reinterpret_cast<Ty*>(static_cast<byte*>(memory_block) +
                                       sizeof(ref_counter_t))};
  shared_ptr<Ty> sptr(object_ptr,
                      new (memory_block) ref_counter_t(
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
      mm::details::ref_counter_with_allocator_deallocate_itself<Ty, AlTy>;

  constexpr size_t summary_size{sizeof(Ty) + sizeof(ref_counter_t)};
  auto* memory_block{allocator_traits<AlTy>::allocate_bytes(
      alloc, summary_size)};  // Alloc должен предоставить allocate_bytes()
  Ty* object_ptr{reinterpret_cast<Ty*>(static_cast<byte*>(memory_block) +
                                       sizeof(ref_counter_t))};

  auto* ref_counter{construct_at(static_cast<ref_counter_t*>(memory_block),
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
}  // namespace ktl

#endif
