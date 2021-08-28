#pragma once
// RVA is Relative Virtual Address

#include <../basic_types.hpp>
#include <../crt_assert.hpp>
#include <../crt_attributes.hpp>
#include <../algorithm_impl.hpp>
#include <../limits_impl.hpp>

#include <member_ptr.hpp>

namespace ktl::crt::exc_engine {
template <typename Ty, typename IntegralTy>
Ty convert_narrow(IntegralTy value) noexcept {
  crt_assert((numeric_limits<Ty>::min)() <= value &&
             value <= (numeric_limits<Ty>::max)());
  return static_cast<Ty>(value);
}

template <typename Ty>
struct relative_virtual_address {
  constexpr relative_virtual_address() = default;

  constexpr relative_virtual_address(offset_t offset) noexcept
      : m_offset{offset} {}

  relative_virtual_address(Ty* ptr, const void* base) noexcept
      : m_offset{calculate_offset(ptr, base)} {}

  constexpr explicit operator bool() const noexcept {
    return static_cast<bool>(m_offset);
  }

  constexpr offset_t value() const { return m_offset; }

  constexpr relative_virtual_address& operator+=(offset_t rhs) noexcept {
    m_offset += rhs;
    return *this;
  }

  template <typename OtherTy,
            enable_if_t<is_convertible_v<OtherTy*, Ty*>, int> = 0>
  constexpr operator relative_virtual_address<OtherTy>() const noexcept {
    return relative_virtual_address<OtherTy>{m_offset};
  }

  constexpr friend Ty* operator+(const void* base,
                                 relative_virtual_address<Ty> rva) noexcept {
    return reinterpret_cast<Ty*>(reinterpret_cast<uintptr_t>(base) +
                                 rva.value());
  }

  constexpr friend const byte* operator-(
      Ty* ptr,
      relative_virtual_address<Ty> rva) noexcept {
    return reinterpret_cast<const byte*>(reinterpret_cast<uintptr_t>(ptr) -
                                         rva.value());
  }

  constexpr friend relative_virtual_address operator+(
      relative_virtual_address lhs,
      offset_t rhs) noexcept {
    return relative_virtual_address{lhs.m_offset} += rhs;
  }

  constexpr friend bool operator==(
      const relative_virtual_address& lhs,
      const relative_virtual_address& rhs) noexcept {
    return lhs.m_offset == rhs.m_offset;
  }

  constexpr friend bool operator!=(
      relative_virtual_address const& lhs,
      relative_virtual_address const& rhs) noexcept {
    return !(lhs == rhs);
  }

  constexpr friend bool operator<(
      relative_virtual_address const& lhs,
      relative_virtual_address const& rhs) noexcept {
    return lhs.m_offset < rhs.m_offset;
  }

  constexpr friend bool operator<=(
      relative_virtual_address const& lhs,
      relative_virtual_address const& rhs) noexcept {
    return !(rhs < lhs);
  }

  constexpr friend bool operator>(
      relative_virtual_address const& lhs,
      relative_virtual_address const& rhs) noexcept {
    return rhs < lhs;
  }

  constexpr friend bool operator>=(
      relative_virtual_address const& lhs,
      relative_virtual_address const& rhs) noexcept {
    return !(lhs < rhs);
  }

  static offset_t calculate_offset(Ty* ptr, const void* base) noexcept {
    return convert_narrow<offset_t>(reinterpret_cast<uintptr_t>(ptr) -
                                    reinterpret_cast<uintptr_t>(base));
  }

  offset_t m_offset{0};  // Не private, т.к. __GSHandlerCheck() обращается к полю
                         // напрямую, а для private-полей такой доступ - UB
};

template <typename Ty>
relative_virtual_address<Ty> make_rva(Ty* ptr, const void* base) noexcept {
  return relative_virtual_address<Ty>{ptr, base};
}

}  // namespace ktl::crt::exc_engine