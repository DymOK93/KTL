#pragma once
#include <assert.hpp>
#include <hash.hpp>
#include <ktlexcept.hpp>
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
  using MyBase = exception;

  constexpr bad_optional_access() noexcept : MyBase{"optional is empty"} {}

  [[nodiscard]] NTSTATUS code() const noexcept final {
    return STATUS_END_OF_FILE;
  }
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
      (is_trivially_copyable_v<Ty> && is_trivially_destructible_v<Ty> &&
       !is_const_v<Ty> && !is_volatile_v<Ty>);
};

template <class Ty>
inline constexpr bool supported_by_trivial_optional_base_v =
    supported_by_trivial_optional_base<Ty>::value;

#define INTERNAL_STORAGE            \
  bool m_initialized{false};        \
  union {                           \
    non_trivial_dummy_type m_dummy; \
    Ty m_value;                     \
  };

#define OPTIONAL_STORAGE_CONSTRUCTOR_SET                                     \
  constexpr optional_storage() noexcept : m_dummy{} {}                       \
                                                                             \
  constexpr optional_storage(nullopt_t) noexcept : optional_storage() {}     \
                                                                             \
  constexpr optional_storage(const Ty& value) noexcept                       \
      : m_initialized{true}, m_value{value} {}                               \
                                                                             \
  constexpr optional_storage(Ty&& value) noexcept                            \
      : m_initialized{true}, m_value{move(value)} {}                         \
                                                                             \
  template <class U = Ty, enable_if_t<is_constructible_v<Ty, U>, int> = 0>   \
  constexpr optional_storage(U&& value) noexcept(                            \
      is_nothrow_constructible_v<Ty, U>)                                     \
      : m_initialized{true}, m_value{forward<U>(value)} {}                   \
                                                                             \
  template <class... Types>                                                  \
  constexpr explicit optional_storage(in_place_t, Types&&... args) noexcept( \
      is_nothrow_constructible_v<Ty, Types...>)                              \
      : m_initialized{true}, m_value(forward<Types>(args)...) {}

template <class Ty, bool = supported_by_trivial_optional_base_v<Ty>>
class optional_storage {
 public:
  using value_type = Ty;

 public:
  OPTIONAL_STORAGE_CONSTRUCTOR_SET
  ~optional_storage() noexcept {}

 protected:
  INTERNAL_STORAGE
};

template <class Ty>
class optional_storage<Ty, true> {
 public:
  using value_type = Ty;

 public:
  OPTIONAL_STORAGE_CONSTRUCTOR_SET
  ~optional_storage() noexcept = default;

 protected:
  INTERNAL_STORAGE
};

#undef OPTIONAL_STORAGE_CONSTRUCTOR_SET
#undef INTERNAL_STORAGE

template <class Ty>
class common_optional_base : public optional_storage<Ty> {
 private:
  using MyBase = optional_storage<Ty>;

 public:
  using value_type = typename MyBase::value_type;

 public:
  using MyBase::MyBase;

  constexpr explicit operator bool() const noexcept { return has_value(); }
  [[nodiscard]] constexpr bool has_value() const noexcept {
    return this->m_initialized;
  }

  constexpr const Ty* operator->() const noexcept { return get_ptr(); }
  constexpr Ty* operator->() noexcept { return get_ptr(); }

 protected:
  constexpr Ty& get_ref() noexcept { return this->m_value; }

  constexpr const Ty& get_ref() const noexcept { return this->m_value; }

  constexpr Ty* get_ptr() noexcept { return addressof(this->m_value); }

  constexpr const Ty* get_ptr() const noexcept {
    return addressof(this->m_value);
  }

  constexpr void construct_from_value(const Ty& value) noexcept(
      is_nothrow_copy_constructible_v<Ty>) {
    this->m_value = value;
    this->m_initialized = true;
  }

  template <class U>
  constexpr void construct_from_value(const U& value) noexcept(
      is_nothrow_constructible_v<Ty, U>) {
    this->m_value = static_cast<value_type>(value);
    this->m_initialized = true;
  }

  template <class... Types>
  constexpr void construct_from_args(Types&&... args) noexcept(
      is_nothrow_constructible_v<Ty, Types...>) {
    construct_at(get_ptr(), forward<Types>(args)...);
    this->m_initialized = true;
  }
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

  constexpr trivial_optional_base(trivial_optional_base&&) noexcept = default;

  template <class U, enable_if_t<is_convertible_v<U, Ty>, int> = 0>
  constexpr trivial_optional_base(
      const trivial_optional_base<U>&
          other) noexcept(is_nothrow_convertible_v<U, Ty>) {
    if (other.has_value()) {
      MyBase::construct_from_value(other.get_ref());
    }
  }

  constexpr trivial_optional_base& operator=(nullopt_t) noexcept {
    reset();
    return *this;
  }

  constexpr trivial_optional_base& operator=(
      const trivial_optional_base&) noexcept = default;

  constexpr trivial_optional_base& operator=(trivial_optional_base&&) noexcept =
      default;

  template <class U = Ty, enable_if_t<is_convertible_v<U, Ty>, int> = 0>
  constexpr trivial_optional_base&
  operator=(const trivial_optional_base<U>& other) noexcept(
      is_nothrow_convertible_v<U, Ty>) {
    if (!other.has_value()) {
      reset();
    } else {
      MyBase::construct_from_value(*other);
    }
    return *this;
  }

  template <class U = Ty, enable_if_t<is_convertible_v<U, Ty>, int> = 0>
  constexpr trivial_optional_base& operator=(U&& value) {
    MyBase::construct_from_value(forward<U>(value));
    return *this;
  }

  ~trivial_optional_base() noexcept = default;

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
      MyBase::construct_from_args(*other);
    }
  }

  non_trivial_optional_base(non_trivial_optional_base&& other) noexcept(
      is_nothrow_move_constructible_v<Ty>) {
    if (other.has_value()) {
      MyBase::construct_from_args(*move(other));
    }
  }

  template <
      typename U,
      enable_if_t<is_constructible_v<Ty, add_lvalue_reference_t<U>>, int> = 0>
  non_trivial_optional_base(const non_trivial_optional_base<U>& other) noexcept(
      is_nothrow_constructible_v<Ty, add_lvalue_reference_t<U>>) {
    if (other.has_value()) {
      MyBase::construct_from_value(*other);
    }
  }

  template <
      typename U,
      enable_if_t<is_constructible_v<Ty, add_rvalue_reference_t<U>>, int> = 0>
  non_trivial_optional_base(non_trivial_optional_base<U>&& other) noexcept(
      is_nothrow_constructible_v<Ty, add_rvalue_reference_t<U>>) {
    if (other.has_value()) {
      MyBase::construct_from_value(*move(other));
    }
  }

  ~non_trivial_optional_base() noexcept { reset_if_needed(); }

  constexpr non_trivial_optional_base& operator=(nullopt_t) noexcept {
    reset();
    return *this;
  }

  non_trivial_optional_base&
  operator=(const non_trivial_optional_base& other) noexcept(
      is_nothrow_copy_constructible_v<Ty>&& is_nothrow_copy_assignable_v<Ty>) {
    if (addressof(other) != this) {
      if (other.has_value()) {
        assign_or_emplace(*other);
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
        emplace(*other);
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
        emplace(*move(other));
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

  template <typename U, enable_if_t<is_constructible_v<Ty, U>, int> = 0>
  constexpr Ty value_or(U&& default_value) const& {
    return has_value() ? MyBase::get_ref() : forward<U>(default_value);
  }

  template <typename U, enable_if_t<is_constructible_v<Ty, U>, int> = 0>
  constexpr Ty value_or(U&& default_value) && {
    return has_value() ? move(MyBase::get_ref()) : forward<U>(default_value);
  }

  template <class... Types>
  Ty& emplace(Types&&... args) noexcept(
      is_nothrow_constructible_v<Ty, Types...>) {
    reset_if_needed();
    MyBase::construct_from_args(forward<Types>(args)...);
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
class optional;

template <class Ty>
void swap(optional<Ty>& lhs,
          optional<Ty>& rhs) noexcept(is_nothrow_swappable_v<Ty>);

template <class Ty>
class optional : public opt::details::optional_base_selector_t<Ty> {
 private:
  using MyBase = opt::details::optional_base_selector_t<Ty>;

 public:
  using value_type = typename MyBase::value_type;

 public:
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

  void swap(optional& other) noexcept(is_nothrow_swappable_v<Ty>) {
    swap(*this, other);
  }
};

template <class Ty>
optional(Ty) -> optional<Ty>;

template <class Ty>
void swap(optional<Ty>& lhs,
          optional<Ty>& rhs) noexcept(is_nothrow_swappable_v<Ty>) {
  if (bool lhs_not_empty = lhs.has_value();
      (lhs_not_empty ^ rhs.has_value()) == 0) {
    if (lhs_not_empty) {
      swap(*lhs, *rhs);
    }
  } else {
    auto& not_empty{lhs_not_empty ? lhs : rhs};
    auto& empty{lhs_not_empty ? rhs : lhs};
    empty.emplace(move(*not_empty));
    not_empty.reset();
  }
}

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
  [[nodiscard]] size_t operator()(const optional<Ty>& opt) const
      noexcept(noexcept(hash<remove_const_t<Ty>>{}(*opt))) {
    return hash<remove_const_t<Ty>>{}(*opt);
  }
};

template <class Ty1, class Ty2>
constexpr bool operator==(
    const optional<Ty1>& lhs,
    const optional<Ty2>& rhs) noexcept(noexcept(declval<Ty1>() ==
                                                declval<Ty2>())) {
  bool lhs_not_empty{lhs.has_value()};
  if ((lhs_not_empty ^ rhs.has_value()) != 0) {
    return false;
  }
  return !lhs_not_empty ? true : *lhs == *rhs;
}

template <class Ty1, class Ty2>
constexpr bool operator!=(const optional<Ty1>& lhs,
                          const optional<Ty2>& rhs) noexcept(noexcept(lhs ==
                                                                      rhs)) {
  return !(lhs == rhs);
}

template <class Ty1, class Ty2>
constexpr bool operator<(
    const optional<Ty1>& lhs,
    const optional<Ty2>& rhs) noexcept(noexcept(declval<Ty1>() <
                                                declval<Ty2>())) {
  if (!rhs) {
    return false;
  }
  return !lhs ? true : *lhs < *rhs;
}

template <class Ty1, class Ty2>
constexpr bool operator<=(const optional<Ty1>& lhs,
                          const optional<Ty2>& rhs) noexcept(noexcept(lhs >
                                                                      rhs)) {
  return !(lhs > rhs);
}

template <class Ty1, class Ty2>
constexpr bool operator>(const optional<Ty1>& lhs,
                         const optional<Ty2>& rhs) noexcept(noexcept(rhs <
                                                                     lhs)) {
  return rhs < lhs;
}

template <class Ty1, class Ty2>
constexpr bool operator>=(const optional<Ty1>& lhs,
                          const optional<Ty2>& rhs) noexcept(noexcept(rhs <
                                                                      lhs)) {
  return !(lhs < rhs);
}

template <class Ty, class U>
constexpr bool operator==(const optional<Ty>& opt,
                          const U& value) noexcept(noexcept(declval<Ty>() ==
                                                            declval<U>())) {
  return !opt ? false : *opt == value;
}

template <class Ty, class U>
constexpr bool operator==(const Ty& value, const optional<U>& opt) noexcept(
    noexcept(declval<Ty>() == declval<U>())) {
  return !opt ? false : value == *opt;
}

template <class Ty, class U>
constexpr bool operator!=(const optional<Ty>& opt,
                          const U& value) noexcept(noexcept(declval<Ty>() !=
                                                            declval<U>())) {
  return !opt ? false : *opt != value;
}

template <class Ty, class U>
constexpr bool operator!=(const Ty& value, const optional<U>& opt) noexcept(
    noexcept(declval<Ty>() != declval<U>())) {
  return !opt ? false : value != *opt;
}

template <class Ty, class U>
constexpr bool operator<(const optional<Ty>& opt,
                         const U& value) noexcept(noexcept(declval<Ty>() <
                                                           declval<U>())) {
  return !opt ? false : *opt < value;
}

template <class Ty, class U>
constexpr bool operator<(const Ty& value, const optional<U>& opt) noexcept(
    noexcept(declval<Ty>() < declval<U>())) {
  return !opt ? false : value < *opt;
}

template <class Ty, class U>
constexpr bool operator<=(const optional<Ty>& opt,
                          const U& value) noexcept(noexcept(declval<Ty>() <=
                                                            declval<U>())) {
  return !opt ? false : *opt <= value;
}

template <class Ty, class U>
constexpr bool operator<=(const Ty& value, const optional<U>& opt) noexcept(
    noexcept(declval<Ty>() <= declval<U>())) {
  return !opt ? false : value <= *opt;
}

template <class Ty, class U>
constexpr bool operator>(const optional<Ty>& opt,
                         const U& value) noexcept(noexcept(declval<Ty>() >
                                                           declval<U>())) {
  return !opt ? false : *opt > value;
}

template <class Ty, class U>
constexpr bool operator>(const Ty& value, const optional<U>& opt) noexcept(
    noexcept(declval<Ty>() > declval<U>())) {
  return !opt ? false : value > *opt;
}

template <class Ty, class U>
constexpr bool operator>=(const optional<Ty>& opt,
                          const U& value) noexcept(noexcept(declval<Ty>() >=
                                                            declval<U>())) {
  return !opt ? false : *opt >= value;
}
template <class Ty, class U>
constexpr bool operator>=(const Ty& value, const optional<U>& opt) noexcept(
    noexcept(declval<Ty>() >= declval<U>())) {
  return !opt ? false : value >= *opt;
}
}  // namespace ktl
