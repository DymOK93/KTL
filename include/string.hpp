#pragma once
#include <new_delete.h>
#include <allocator.hpp>
#include <char_traits_impl.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

#include <ntddk.h>

namespace ktl {
template <size_t SsoBufferChCount,
          template <typename... CharT>
          class Traits,
          template <typename... CharT>
          class Alloc>
class basic_unicode_string {
 public:
  using native_str_t = UNICODE_STRING;

 public:
  using value_type = wchar_t;
  using traits_type = Traits<value_type>;
  using allocator_type = Alloc<value_type>;
  using allocator_traits_type = allocator_traits<allocator_type>;
  using size_type = uint16_t;  // Ограничение unicode_string
  using difference_type = typename allocator_traits_type::difference_type;
  using reference = typename allocator_traits<allocator_type>::reference;
  using const_reference = typename allocator_traits_type::const_reference;
  using pointer = typename allocator_traits_type::pointer;
  using const_pointer = typename allocator_traits_type::const_pointer;
  using iterator = pointer;
  using const_iterator = const_pointer;

 private:
  static constexpr size_type GROWTH_MULTIPLIER{2};

 public:
  static constexpr auto npos{static_cast<size_type>(-1)};

 public:
  constexpr basic_unicode_string() noexcept { become_small(); }

  constexpr explicit basic_unicode_string(const allocator_type& alloc) noexcept(
      is_nothrow_copy_constructible_v<allocator_type>)
      : m_alc(alloc) {
    become_small();
  }

  constexpr explicit basic_unicode_string(allocator_type&& alloc) noexcept(
      is_nothrow_move_constructible_v<allocator_type>)
      : m_alc(move(alloc)) {
    become_small();
  }

  template <size_type N, class Allocator = allocator_type>
  basic_unicode_string(const value_type (&null_terminated_str)[N],
                       Allocator&& alloc = Allocator{})
      : m_alc(forward<Allocator>(alloc)) {
    become_small();
    assign(null_terminated_str, N - 1);  // N учитывает нуль-символ
  }

  basic_unicode_string(const basic_unicode_string& other) = delete;

  basic_unicode_string(basic_unicode_string&& other) noexcept
      : m_alc{move(other.m_alc)}, m_str{other.m_str} {
    if (other.is_small()) {
      m_str.Buffer = m_buffer;
      traits_type::copy(data(), other.data(), other.size());
    }
    other.become_small();
  }

  ~basic_unicode_string() noexcept {
    if (!is_small()) {
      deallocate_buffer(get_alloc(), data(), capacity());
    }
  }

  basic_unicode_string& assign(size_type count, value_type ch) {
    clear();
    resize(count, ch);
    return *this;
  }

  basic_unicode_string& assign(const value_type* str, size_type ch_count) {
    reserve(ch_count);
    traits_type::move(data(), str, ch_count);  // str может быть частью data()
    m_str.Length = ch_count_to_bytes(ch_count);
    return *this;
  }

  void reserve(size_type new_capacity = 0) {
    if (new_capacity > size()) {
      auto& alc{get_alloc()};
      value_type* old_buffer{data()};
      value_type* new_buffer{new_capacity <= SsoBufferChCount
                                 ? m_buffer
                                 : allocate_buffer(alc, new_capacity)};
      traits_type::copy(new_buffer, old_buffer, size());
      if (!is_small()) {
        deallocate_buffer(alc, old_buffer, capacity());
      }
      m_str.Buffer = new_buffer;
      m_str.MaximumLength = ch_count_to_bytes(new_capacity);
    }
  }

  void resize(size_type count) { resize(count, value_type{}); }

  void resize(size_type count, value_type ch) {
    reserve(count);
    if (size() < count) {
      traits_type::assign(data(), count - size(), ch);
    }
    m_str.Length = ch_count_to_bytes(count);
  }

  void clear() noexcept { m_str.Length = 0; }

  constexpr value_type* data() noexcept { return m_str.Buffer; }

  constexpr const value_type* data() const noexcept { return m_str.Buffer; }

  constexpr size_type size() const noexcept {
    return bytes_count_to_ch(m_str.Length);
  }

  constexpr size_type length() const noexcept {
    return bytes_count_to_ch(m_str.Length);
  }

  constexpr size_type capacity() const noexcept {
    return bytes_count_to_ch(m_str.MaximumLength);
  }

  constexpr size_type max_size() const noexcept { return npos - 1; }

  constexpr native_str_t* raw_str() noexcept { return addressof(m_str); }

  constexpr const native_str_t* raw_str() const noexcept {
    return addressof(m_str);
  }

 private:
  constexpr void become_small(size_type size = 0) noexcept {
    m_str.Buffer = m_buffer;
    m_str.Length = size;
    m_str.MaximumLength = SsoBufferChCount;
  }

  constexpr bool is_small() const noexcept {
    return capacity() <= SsoBufferChCount;
  }

  constexpr allocator_type& get_alloc() noexcept { return m_alc; }

  static constexpr size_type bytes_count_to_ch(size_type bytes_count) noexcept {
    return bytes_count / sizeof(value_type);
  }

  static constexpr size_type ch_count_to_bytes(size_type ch_count) noexcept {
    return ch_count * sizeof(value_type);
  }

  static value_type* allocate_buffer(allocator_type& alc, size_type ch_count) {
    return allocator_traits_type::allocate(alc, ch_count);
  }

  static void deallocate_buffer(allocator_type& alc,
                                value_type* buffer,
                                size_type ch_count) noexcept {
    return allocator_traits_type::deallocate(alc, buffer, ch_count);
  }

  static constexpr size_type calc_growth(size_type old_capacity) noexcept {
    return old_capacity * GROWTH_MULTIPLIER;
  }

 private:
  allocator_type m_alc{};
  native_str_t m_str{};
  value_type m_buffer[SsoBufferChCount]{value_type{}};
};

using unicode_string =
    basic_unicode_string<16, char_traits, basic_paged_allocator>;
using unicode_string_non_paged =
    basic_unicode_string<16, char_traits, basic_non_paged_allocator>;
}  // namespace ktl