#pragma once

#ifndef NO_CXX_STANDARD_LIBRARY
#include <memory>
namespace winapi::kernel::mm {
using std::unique_ptr;
}  // namespace winapi::kernel
#else
#include <heap.h>
#include <type_traits.hpp>
#include <utility.hpp>
#include <functional.hpp>

namespace winapi::kernel::mm {  // memory management
namespace details {
template <class Ty>
struct default_delete {
  static constexpr bool enable_delete_null =
      true;  //Не требует проверки на nullptr перед вызовом

  constexpr default_delete() = default;

  template <class OtherTy, enable_if_t<is_convertible_v<OtherTy, Ty>, int> = 0>
  default_delete(const default_delete<OtherTy>&) noexcept {}

  void operator()(Ty* ptr) const noexcept {
    static_assert(sizeof(Ty) > 0, "can't delete an incomplete type");
    delete ptr;
  }
};
template <class Ty>
struct default_delete<Ty[]> {
  static constexpr bool enable_delete_null =
      true;  //Не требует проверки на nullptr перед вызовом

  constexpr default_delete() = default;

  template <class OtherTy, enable_if_t<is_convertible_v<OtherTy, Ty>, int> = 0>
  default_delete(const default_delete<OtherTy>&) noexcept {}

  void operator()(Ty* ptr) const noexcept {
    static_assert(sizeof(Ty) > 0, "can't delete an incomplete type");
    delete ptr;
  }
};

template <class Ty, class Deleter, class = void>
struct get_deleter_pointer_type {
  using type = add_pointer_t<Ty>;
};

template <class Ty, class Deleter>
struct get_deleter_pointer_type<Ty,
                                Deleter,
                                void_t<typename Deleter::pointer> > {
  using type = typename Deleter::pointer;
};

template <class Ty, class Deleter>
using deleter_pointer_t = typename get_deleter_pointer_type<Ty, Deleter>::type;

template <class Deleter, class = void>
struct enable_delete_null : false_type {};

template <class Deleter>
struct enable_delete_null<Deleter,
                          void_t<decltype(Deleter::enable_delete_null)> > {
  static constexpr bool value = Deleter::enable_delete_null;
};

template <class Deleter>
inline constexpr bool enable_delete_null_v = enable_delete_null<Deleter>::value;

}  // namespace details

template <class Ty, class Deleter = details::default_delete<Ty> >
class unique_ptr {
 public:
  using pointer = details::deleter_pointer_t<Ty, remove_reference_t<Deleter> >;
  using element_type = Ty;
  using deleter_type = Deleter;

 public:
  template <class Dx = Deleter,
            enable_if_t<!is_pointer_v<Dx> &&  //Нельзя хранить nullptr
                                              //в качестве deleter'a
                            is_default_constructible_v<Dx>,
                        int> = 0>
  constexpr unique_ptr() noexcept : m_ptr{nullptr} {}

  constexpr unique_ptr(nullptr_t) noexcept;
  explicit unique_ptr(pointer ptr) noexcept : m_ptr{ptr} {}

  template <class Dx = Deleter,
            enable_if_t<is_constructible_v<Dx, const Dx&>, int> = 0>
  unique_ptr(pointer ptr,
             const Deleter& deleter) noexcept(is_nothrow_constructible_v<Dx>)
      : m_ptr{ptr}, m_deleter(deleter) {}

  // Dx - не ссылочный тип
  template <
      class Dx = Deleter,
      enable_if_t<!is_reference_v<Dx> && is_move_constructible_v<Dx>, int> = 0>
  unique_ptr(pointer ptr,
             Deleter&& deleter) noexcept(is_nothrow_move_constructible_v<Dx>)
      : m_ptr{ptr}, m_deleter(move(deleter)) {}

  // Dx - ссылочный тип
  template <class Dx = Deleter,
            enable_if_t<is_reference_v<Dx> &&
                            is_constructible_v<Dx, remove_reference_t<Dx> >,
                        int> = 0>
  unique_ptr(pointer ptr, Dx&& deleter) = delete;

  template <class Dx = Deleter,
            enable_if_t<is_move_constructible_v<Dx>, int> = 0>
  unique_ptr(unique_ptr&& other) noexcept(is_nothrow_move_constructible_v<Dx>)
      : m_ptr{exchange(other.m_ptr, nullptr)},
        m_deleter(forward<Dx>(other.get_deleter())) {}

  template <class OtherTy,
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
                                  is_convertible<deleter_type, OtherDeleter> >,
                int> = 0>
  unique_ptr(unique_ptr<OtherTy, OtherDeleter>&& other) noexcept(
      is_nothrow_constructible_v<deleter_type, OtherDeleter>)
      : m_ptr{ptr}, m_deleter(forward<deleter_type>(other.get_deleter())) {}

  template <class Dx = Deleter, enable_if_t<is_move_assignable_v<Dx>, int> = 0>
  unique_ptr& operator=(unique_ptr&& other) noexcept(
      is_nothrow_move_assignable_v<Dx>) {
    if (addressof(other) != this) {
      reset(other.release());
      m_deleter = forward<deleter_type>(other.get_deleter());
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
    m_deleter = forward<deleter_type>(other.get_deleter());
    return *this;
  }

  unique_ptr(const unique_ptr&) = delete;
  unique_ptr& operator=(const unique_ptr&) = delete;

  ~unique_ptr() { reset(); }

  pointer release() noexcept { return exchange(m_ptr, nullptr); }

  void reset(pointer ptr = pointer{}) noexcept {
    pointer target{exchange(m_ptr, ptr)};
    if constexpr (details::enable_delete_null_v<deleter_type>) {  //Поддерживает
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
    swap(m_ptr, other.m_ptr);
    swap(m_deleter, other.m_deleter);
  }

  pointer get() const noexcept { return m_ptr; }

  Deleter& get_deleter() noexcept { return m_deleter; }
  const Deleter& get_deleter() const noexcept { return m_deleter; }

  operator bool() const noexcept { return static_cast<bool>(m_ptr); }

  add_lvalue_reference_t<Ty> operator*()
      const& {  //Перегруженный Ty::operator*() способен вызвать
                //исключение
    return *m_ptr;
  }

  add_rvalue_reference_t<Ty> operator*() const&& { return move(*m_ptr); }

  pointer operator->() const noexcept { return m_ptr; }

 private:
  template <class,
            class>  //Для конструирования из unique_ptr с указателем
                    //другого типа
  friend class unique_ptr;  //(например, для
                            //полиморфных объектов)

  pointer m_ptr;
  deleter_type m_deleter;
};

template <class Ty1, class Dx1, class Ty2, class Dx2>
bool operator==(const unique_ptr<Ty1, Dx1>& lhs,
                const unique_ptr<Ty1, Dx1>& rhs) noexcept {
  return lhs.get() == rhs.get();
}

template <class Ty1, class Dx1, class Ty2, class Dx2>
bool operator!=(const unique_ptr<Ty1, Dx1>& lhs,
                const unique_ptr<Ty1, Dx1>& rhs) noexcept {
  return !(lhs == rhs);
}

template <class Ty1, class Dx1, class Ty2, class Dx2>
bool operator<(const unique_ptr<Ty1, Dx1>& lhs,
               const unique_ptr<Ty1, Dx1>& rhs) noexcept {
  using common_t = common_type_t<typename unique_ptr<Ty1, Dx1>::pointer,
                                 typename unique_ptr<Ty2, Dx2>::pointer>;
  return less<common_t>(lhs.get(), rhs.get());
}

template <class Ty1, class Dx1, class Ty2, class Dx2>
bool operator>(const unique_ptr<Ty1, Dx1>& lhs,
               const unique_ptr<Ty1, Dx1>& rhs) noexcept {
  using common_t = common_type_t<typename unique_ptr<Ty1, Dx1>::pointer,
                                 typename unique_ptr<Ty2, Dx2>::pointer>;
  return greater<common_t>(lhs.get(), rhs.get());
}

template <class Ty1, class Dx1, class Ty2, class Dx2>
bool operator<=(const unique_ptr<Ty1, Dx1>& lhs,
                const unique_ptr<Ty1, Dx1>& rhs) noexcept {
  return !(lhs > rhs);
}

template <class Ty1, class Dx1, class Ty2, class Dx2>
bool operator>=(const unique_ptr<Ty1, Dx1>& lhs,
                const unique_ptr<Ty1, Dx1>& rhs) noexcept {
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
  using common_t = common_type_t<typename unique_ptr<Ty1, Dx1>::pointer,
                                 typename unique_ptr<Ty2, Dx2>::pointer>;
  return less<common_t>(ptr.get(), null);
}

template <class Ty, class Dx>
bool operator>(const unique_ptr<Ty, Dx>& ptr, nullptr_t null) noexcept {
  using common_t = common_type_t<typename unique_ptr<Ty1, Dx1>::pointer,
                                 typename unique_ptr<Ty2, Dx2>::pointer>;
  return greater<common_t>(ptr.get(), null);
}

template <class Ty, class Dx>
bool operator<(nullptr_t null, const unique_ptr<Ty, Dx>& ptr) noexcept {
  using common_t = common_type_t<typename unique_ptr<Ty1, Dx1>::pointer,
                                 typename unique_ptr<Ty2, Dx2>::pointer>;
  return less<common_t>(null, ptr.get());
}

template <class Ty, class Dx>
bool operator>(nullptr_t null, const unique_ptr<Ty, Dx>& ptr) noexcept {
  using common_t = common_type_t<typename unique_ptr<Ty1, Dx1>::pointer,
                                 typename unique_ptr<Ty2, Dx2>::pointer>;
  return greater<common_t>(null, ptr.get());
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

template <class Ty, class... Types>
unique_ptr<Ty> make_unique(Types&&... args) {
  return unique_ptr<Ty>(new (nothrow) Ty(forward<Types>(args)...));
}

}  // namespace winapi::kernel::mm

namespace std {
template <class Ty, class Deleter>
void swap(winapi::kernel::mm::unique_ptr<Ty, Deleter>& lhs,
          winapi::kernel::mm::unique_ptr<Ty, Deleter>&
              rhs) noexcept(noexcept(lhs.swap(rhs))) {
  lhs.swap(rhs);
}
}  // namespace std
#endif