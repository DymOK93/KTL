#pragma once
#include <exception.h>
#include <assert.hpp>
#include <hash.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

namespace ktl {
struct nullopt_t {
  /*
   * nullopt_t must be a non-aggregate LiteralType
   * and cannot have a default constructor or an initializer-list constructor
   */
  explicit constexpr nullopt_t(int) noexcept {}
};

inline constexpr nullopt_t nullopt{0};

struct bad_optional_access : exception {
 public:
  using MyBase = exception;

 public:
  constexpr bad_optional_access() noexcept
      : MyBase{L"optional is empty", constexpr_message_tag{}} {}

  NTSTATUS code() const noexcept override final { return STATUS_END_OF_FILE; }
};

namespace opt::details {
struct non_trivial_dummy_type {
  /*
   * This default constructor is user-provided to avoid zero-initialization
   * when objects are value-initialized
   */
  constexpr non_trivial_dummy_type() noexcept {}
};

template <class Ty>
struct supported_by_trivial_optional_base {
  static constexpr bool value =
      (is_trivially_default_constructible_v<Ty> &&
       is_trivially_copyable_v<Ty> && is_trivially_destructible_v<Ty> &&
       !is_const_v<Ty> && !is_volatile_v<Ty>);
};

template <class Ty>
inline constexpr bool supported_by_trivial_optional_base_v =
    supported_by_trivial_optional_base<Ty>::value;

template <class Ty>
class common_optional_base {
 public:
  using value_type = Ty;

 public:
  constexpr common_optional_base() noexcept : m_dummy{} {}

  constexpr common_optional_base(nullopt_t) noexcept : common_optional_base() {}

  constexpr common_optional_base(const Ty& value) noexcept
      : m_initialized{true}, m_value{value} {}

  constexpr common_optional_base(Ty&& value) noexcept
      : m_initialized{true}, m_value{move(value)} {}

  template <class U = Ty,
            enable_if_t<supported_by_trivial_optional_base_v<Ty> &&
                                is_convertible_v<U, Ty> ||
                            is_constructible_v<Ty, U>,
                        int> = 0>
  constexpr common_optional_base(U&& value) noexcept(
      supported_by_trivial_optional_base_v<Ty>&&
          is_nothrow_convertible_v<U, Ty> ||
      is_nothrow_constructible_v<Ty, U>)
      : m_initialized{true}, m_value{forward<U>(value)} {}

  template <class... Types>
  constexpr explicit common_optional_base(in_place_t, Types&&... args) noexcept(
      is_nothrow_constructible_v<Ty, Types...>)
      : m_initialized{true}, m_value(forward<Types>(args)...) {}

  constexpr ~common_optional_base() noexcept {}

  constexpr explicit operator bool() const noexcept { return has_value(); }
  constexpr bool has_value() const noexcept { return m_initialized; }

  constexpr const Ty* operator->() const noexcept { return get_ptr(); }
  constexpr Ty* operator->() noexcept { return get_ptr(); }

 protected:
  constexpr Ty& get_ref() noexcept { return m_value; }
  constexpr const Ty& get_ref() const noexcept { return m_value; }
  constexpr Ty* get_ptr() noexcept { return addressof(m_value); }
  constexpr const Ty* get_ptr() const noexcept { return addressof(m_value); }

  constexpr void construct_from_value(const Ty& value) noexcept {
    m_initialized = true;
    m_value = value;
  }

  template <class U>
  constexpr void construct_from_value(const U& value) noexcept {
    m_initialized = true;
    m_value = static_cast<value_type>(value);
  }

  template <class... Types>
  constexpr void construct_from_args(Types&&... args) noexcept {
    m_initialized = true;
    construct_at(get_ptr(), forward<Types>(args)...);
  }

 protected:
  bool m_initialized{false};
  union {
    non_trivial_dummy_type m_dummy;
    Ty m_value;
  };
};

template <class Ty>
class trivial_optional_base : public common_optional_base<Ty> {
 private:
  using MyBase = common_optional_base<Ty>;

 public:
  using value_type = typename MyBase::value_type;

 public:
  using MyBase::MyBase;

  constexpr trivial_optional_base(const trivial_optional_base&) noexcept =
      default;

  template <class U, enable_if_t<is_convertible_v<U, Ty>, int> = 0>
  constexpr trivial_optional_base(
      const trivial_optional_base<U>&
          other) noexcept(is_nothrow_convertible_v<U, Ty>) {
    if (other.has_value()) {
      MyBase::construct_from_value(other.get_ref());
    }
  }

  constexpr trivial_optional_base& operator=(nullopt_t) noexcept { reset(); }

  constexpr trivial_optional_base& operator=(
      const trivial_optional_base&) noexcept = default;

  template <class U = Ty, enable_if_t<is_convertible_v<U, Ty>, int> = 0>
  constexpr trivial_optional_base&
  operator=(const trivial_optional_base<U>& other) noexcept(
      is_nothrow_convertible_v<U, Ty>) {
    if (!other.has_value()) {
      reset();
    } else {
      MyBase::construct_from_value(other.get_ref());
    }
    return *this;
  }

  template <class U = Ty, enable_if_t<is_convertible_v<U, Ty>, int> = 0>
  constexpr trivial_optional_base& operator=(U&& value) {
    MyBase::construct_from_value(forward<U>(value));
    return *this;
  }

  using MyBase::has_value;
  using MyBase::operator bool;

  using MyBase::operator->;
  constexpr Ty& operator*() noexcept { return MyBase::get_ref(); }
  constexpr const Ty& operator*() const noexcept { return MyBase::get_ref(); }

  constexpr Ty& value() & {
    throw_exception_if_not<bad_optional_access>(has_value());
    return MyBase::get_ref();
  }

  constexpr const Ty& value() const& {
    throw_exception_if_not<bad_optional_access>(has_value());
    return MyBase::get_ref();
  }

  template <class U, enable_if_t<is_convertible_v<U, Ty>, int> = 0>
  constexpr Ty value_or(U&& default_value) const {
    return has_value() ? MyBase::get_ref() : forward<U>(default_value);
  }

  template <class... Types>
  Ty& emplace(Types&&... args) noexcept(
      is_nothrow_constructible_v<Ty, Types...>) {
    MyBase::construct_from_args(forward<Types>(args)...);
    return MyBase::get_ref();
  }

  constexpr void reset() noexcept { this->m_initialized = false; }
};

template <class Ty>
class non_trivial_optional_base : public common_optional_base<Ty> {
 private:
  using MyBase = common_optional_base<Ty>;

 public:
  using value_type = typename MyBase::value_type;

 public:
  using MyBase::MyBase;

  non_trivial_optional_base(const non_trivial_optional_base& other) noexcept(
      is_nothrow_copy_constructible_v<Ty>) {
    if (other.has_value()) {
      MyBase::construct_from_args(other.get_ref());
    }
  }

  non_trivial_optional_base(non_trivial_optional_base&& other) noexcept(
      is_nothrow_move_constructible_v<Ty>) {
    if (other.has_value()) {
      MyBase::construct_from_args(move(other.get_ref()));
    }
  }

  template <
      typename U,
      enable_if_t<is_constructible_v<Ty, add_lvalue_reference_t<U>>, int> = 0>
  non_trivial_optional_base(const non_trivial_optional_base<U>& other) noexcept(
      is_nothrow_constructible_v<Ty, add_lvalue_reference_t<U>>) {
    if (other.has_value()) {
      MyBase::construct_from_value(other.get_ref());
    }
  }

  template <
      typename U,
      enable_if_t<is_constructible_v<Ty, add_rvalue_reference_t<U>>, int> = 0>
  non_trivial_optional_base(non_trivial_optional_base<U>&& other) noexcept(
      is_nothrow_constructible_v<Ty, add_rvalue_reference_t<U>>) {
    if (other.has_value()) {
      MyBase::construct_from_value(move(other.get_ref()));
    }
  }

  constexpr ~non_trivial_optional_base() noexcept { reset_if_needed(); }

  constexpr non_trivial_optional_base& operator=(nullopt_t) noexcept {
    reset();
  }

  non_trivial_optional_base&
  operator=(const non_trivial_optional_base& other) noexcept(
      is_nothrow_copy_constructible_v<Ty>&& is_nothrow_copy_assignable_v<Ty>) {
    if (addressof(other) != this) {
      if (other.has_value()) {
        assign_or_emplace(other.get_ref());
      } else {
        reset();
      }
    }
    return *this;
  }

  non_trivial_optional_base&
  operator=(non_trivial_optional_base&& other) noexcept(
      is_nothrow_move_constructible_v<Ty>&& is_nothrow_move_assignable_v<Ty>) {
    if (addressof(other) != this) {
      if (other.has_value()) {
        assign_or_emplace(move(other.get_ref()));
      } else {
        reset();
      }
    }
    return *this;
  }

  template <
      typename U = Ty,
      enable_if_t<is_constructible_v<Ty, add_lvalue_reference_t<U>>, int> = 0>
  non_trivial_optional_base&
  operator=(const non_trivial_optional_base<U>& other) noexcept(
      is_nothrow_convertible_v<U, Ty>) {
    if (addressof(other) != this) {
      if (other.has_value()) {
        emplace(other.get_ref());
      } else {
        reset();
      }
    }
    return *this;
  }

  template <
      typename U = Ty,
      enable_if_t<is_constructible_v<Ty, add_rvalue_reference_t<U>>, int> = 0>
  non_trivial_optional_base&
  operator=(non_trivial_optional_base<U>&& other) noexcept(
      is_nothrow_convertible_v<U, Ty>) {
    if (addressof(other) != this) {
      if (other.has_value()) {
        emplace(move(other.get_ref()));
      } else {
        reset();
      }
    }
    return *this;
  }

  template <
      typename U = Ty,
      enable_if_t<is_constructible_v<Ty, U> && is_assignable_v<Ty, U>, int> = 0>
  non_trivial_optional_base& operator=(U&& value) {
    assign_or_emplace(value);
    return *this;
  }

  using MyBase::has_value;
  using MyBase::operator bool;

  using MyBase::operator->;

  constexpr Ty& operator*() & noexcept { return MyBase::get_ref(); }

  constexpr const Ty& operator*() const& noexcept { return MyBase::get_ref(); }

  constexpr Ty&& operator*() && noexcept { return move(MyBase::get_ref()); }

  constexpr const Ty&& operator*() const&& noexcept {
    return MyBase::get_ref();
  }

  Ty& value() & {
    throw_exception_if_not<bad_optional_access>(has_value());
    return MyBase::get_ref();
  }

  const Ty& value() const& {
    throw_exception_if_not<bad_optional_access>(has_value());
    return MyBase::get_ref();
  }

  Ty&& value() && {
    throw_exception_if_not<bad_optional_access>(has_value());
    return move(MyBase::get_ref());
  }

  const Ty&& value() const&& {
    throw_exception_if_not<bad_optional_access>(has_value());
    return MyBase::get_ref();
  }

  template <typename U, enable_if_t<is_convertible_v<U, Ty>, int> = 0>
  constexpr Ty value_or(U&& default_value) const& {
    return has_value() ? MyBase::get_ref() : forward<U>(default_value);
  }

  template <typename U, enable_if_t<is_convertible_v<U, Ty>, int> = 0>
  constexpr Ty value_or(U&& default_value) && {
    return has_value() ? move(MyBase::get_ref()) : forward<U>(default_value);
  }

  template <class... Types>
  Ty& emplace(Types&&... args) noexcept(
      is_nothrow_constructible_v<Ty, Types...>) {
    reset_if_needed();
    construct_from_args(forward<Types>(args)...);
    return MyBase::get_ref();
  }

  constexpr void reset() noexcept {
    reset_if_needed();
    this->m_initialized = false;
  }

 protected:
  template <class Ty>
  void assign_or_emplace(Ty&& value) noexcept {
    if (has_value()) {
      MyBase::get_ref() = forward<Ty>(value);
    } else {
      MyBase::construct_from_args(forward<Ty>(value));
    }
  }

  constexpr void reset_if_needed() noexcept {
    if (has_value()) {
      destroy_at(MyBase::get_ptr());
    }
  }
};

template <class Ty>
struct optional_base_selector {
  using type = conditional_t<supported_by_trivial_optional_base_v<Ty>,
                             trivial_optional_base<Ty>,
                             non_trivial_optional_base<Ty>>;
};

template <class Ty>
using optional_base_selector_t = typename optional_base_selector<Ty>::type;
}  // namespace opt::details

template <class Ty>
struct optional : public opt::details::optional_base_selector_t<Ty> {
  using MyBase = opt::details::optional_base_selector_t<Ty>;
  using value_type = typename MyBase::value_type;

  using MyBase::MyBase;

  using MyBase::operator=;
  using MyBase::operator*;
  using MyBase::operator->;
  using MyBase::operator bool;

  using MyBase::emplace;
  using MyBase::has_value;
  using MyBase::reset;
  using MyBase::value;
  using MyBase::value_or;
};

template <class Ty>
optional(Ty) -> optional<Ty>;

template <class Ty>
constexpr optional<decay_t<Ty>> make_optional(Ty&& value) noexcept(
    is_nothrow_constructible_v<decay_t<Ty>, Ty>) {
  return optional<decay_t<Ty>>{value};
}

template <class Ty, class... Types>
constexpr optional<Ty> make_optional(Types&&... args) noexcept(
    is_nothrow_constructible_v<Ty, Types...>) {
  return optional<Ty>(in_place, forward<Types>(args)...);
}

template <class Ty>
struct hash<optional<Ty>> {
  [[nodiscard]] size_t operator()(const optional<Ty>& opt) const {
    return hash<remove_const_t<Ty>>{}(*opt);
  }
};
}  // namespace ktl
