#pragma once
#include <basic_types.h>

namespace ktl::lockfree {
template <class Ty>
class tagged_pointer {
 public:
  using value_type = Ty;
  using reference = Ty&;
  using pointer = Ty*;

  using placeholder_type = uint64_t;
  using tag_type = uint16_t;

 private:
  static constexpr size_t POINTER_WIDTH{48}, TAG_WIDTH{16};
  static constexpr placeholder_type KERNEL_MODE_ADDRESS_MASK{
      0xFFFF'0000'0000'0000ull};

 private:
  struct compressed_pointer {
    placeholder_type address : POINTER_WIDTH;
    placeholder_type tag : TAG_WIDTH;
  };

 public:
  constexpr tagged_pointer() noexcept = default;

  // TODO: убрать конструктор, реализовать через метод
  explicit tagged_pointer(placeholder_type number) noexcept
      : m_ptr{from_number(number)} {}

  explicit tagged_pointer(pointer ptr, tag_type tag = 0) noexcept
      : m_ptr{pack_ptr(ptr, tag)} {}

  constexpr tagged_pointer(const tagged_pointer&) noexcept = default;
  constexpr tagged_pointer& operator=(const tagged_pointer&) noexcept = default;
  ~tagged_pointer() noexcept = default;

  tag_type get_tag() const noexcept { return extract_tag(m_ptr); }
  tag_type get_next_tag() const noexcept { return get_tag() + 1ull; }

  void set_pointer(pointer ptr) noexcept {
    *this = tagged_pointer{ptr, get_tag()};
  };

  pointer get_pointer() const volatile noexcept { return extract_ptr(m_ptr); }

  placeholder_type get_value() const noexcept { return to_number(m_ptr); }

  reference operator*() noexcept { return *get_pointer(); }
  pointer operator->() noexcept { return get_pointer(); }

  operator bool() const noexcept { return m_ptr.address != 0; }

 private:
  static constexpr Ty* extract_ptr(
      const volatile compressed_pointer& ptr) noexcept {
    return reinterpret_cast<Ty*>(ptr.address | KERNEL_MODE_ADDRESS_MASK);
  }

  static constexpr tag_type extract_tag(
      const volatile compressed_pointer& ptr) noexcept {
    return static_cast<tag_type>(ptr.tag);
  }

  static constexpr compressed_pointer pack_ptr(Ty* addr,
                                               tag_type tag) noexcept {
    compressed_pointer ptr{};
    ptr.address = reinterpret_cast<uint64_t>(addr);
    ptr.tag = tag;
    return ptr;
  }

  static constexpr compressed_pointer from_number(
      placeholder_type number) noexcept {
    compressed_pointer ptr;
    ptr.address = number & ((1ull << POINTER_WIDTH) - 1ull);
    ptr.tag = number >> POINTER_WIDTH;
    return ptr;
  }

  static constexpr placeholder_type to_number(compressed_pointer ptr) noexcept {
    return ptr.address | (ptr.tag << POINTER_WIDTH);
  }

 private:
  compressed_pointer m_ptr{};
};

template <class Ty>
bool operator==(const volatile tagged_pointer<Ty>& lhs,
                const volatile tagged_pointer<Ty>& rhs) noexcept {
  return lhs.get_pointer() == rhs.get_pointer();
}

template <class Ty>
bool operator!=(const volatile tagged_pointer<Ty>& lhs,
                const volatile tagged_pointer<Ty>& rhs) noexcept {
  return !(lhs == rhs);
}
}  // namespace ktl::lockfree