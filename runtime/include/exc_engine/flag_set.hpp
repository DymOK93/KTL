#pragma once
#include <../type_traits_impl.hpp>

namespace ktl::crt::exc_engine {
template <typename Enum>
struct flag_set {
  using value_type = underlying_type_t<Enum>;

  constexpr flag_set() noexcept = default;
  constexpr flag_set(Enum en) noexcept : m_value{static_cast<value_type>(en)} {}
  constexpr explicit flag_set(value_type value) noexcept : m_value{value} {}

  constexpr value_type value() const noexcept { return m_value; }

  constexpr explicit operator bool() const noexcept {
    return static_cast<bool>(m_value);
  }
  constexpr explicit operator value_type() const noexcept { return m_value; }

  constexpr bool has_any_of(flag_set other) const noexcept {
    return m_value & other.m_value;
  }

  template <Enum... flags>
  constexpr value_type bit_intersection() const noexcept {
    constexpr value_type mask{(static_cast<value_type>(flags) | ...)};
    return value() & mask;
  }

  constexpr value_type bit_negation() const noexcept {
    return ~value();
  }

  template <Enum flag>
  constexpr value_type get() const noexcept {
    constexpr value_type mask{static_cast<value_type>(flag)};

    if constexpr ((mask & (mask - 1)) !=
                  0) {  // true, если число - не степень 2
      return (m_value & mask) >> count_trailing_zeros(mask);
    } else {
      return m_value & mask ? 1 : 0;
    }
  }

 private:
  static constexpr size_t count_trailing_zeros(value_type value) {
    size_t result{0};
    while ((value & static_cast<value_type>(1)) == 0) {
      ++result;
      value >>= 1;
    }
    return result;
  }

 private:
  value_type m_value;
};

template <typename Enum>
flag_set<Enum> operator|(flag_set<Enum> lhs, flag_set<Enum> rhs) noexcept {
  return lhs.value() | rhs.value();
}

template <typename Enum>
flag_set<Enum> operator&(flag_set<Enum> lhs, flag_set<Enum> rhs) noexcept {
  return lhs.value() & rhs.value();
}

template <typename Enum>
bool operator==(flag_set<Enum> lhs, flag_set<Enum> rhs) noexcept {
  return lhs.value() == rhs.value();
}

template <typename Enum>
bool operator!=(flag_set<Enum> lhs, flag_set<Enum> rhs) noexcept {
  return !(lhs == rhs);
}
}  // namespace ktl::crt::exc_engine