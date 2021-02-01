#pragma once
#include <new_delete.h>
#include <algorithm.hpp>
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
  using size_type = uint16_t;  // Ограничение UNICODE_STRING
  using difference_type = typename allocator_traits_type::difference_type;
  using reference = typename allocator_traits<allocator_type>::reference;
  using const_reference = typename allocator_traits_type::const_reference;
  using pointer = typename allocator_traits_type::pointer;
  using const_pointer = typename allocator_traits_type::const_pointer;
  using iterator = pointer;
  using const_iterator = const_pointer;

 public:
  static constexpr size_type SSO_BUFFER_CH_COUNT{SsoBufferChCount};

 private:
  static constexpr size_type GROWTH_MULTIPLIER{2};

 public:
  static constexpr auto npos{static_cast<size_type>(-1)};

 public:
  constexpr basic_unicode_string() noexcept { become_small(); }

  explicit constexpr basic_unicode_string(const allocator_type& alloc) noexcept(
      is_nothrow_copy_constructible_v<allocator_type>)
      : m_alc(alloc) {
    become_small();
  }

  explicit constexpr basic_unicode_string(allocator_type&& alloc) noexcept(
      is_nothrow_move_constructible_v<allocator_type>)
      : m_alc(move(alloc)) {
    become_small();
  }

  template <class Allocator = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Allocator>, int> = 0>
  basic_unicode_string(size_type count,
                       value_type ch,
                       Allocator&& alloc = Allocator{})
      : m_alc(forward<Allocator>(alloc)) {
    become_small();
    construct_from_character_unchecked(count, ch);
  }

  basic_unicode_string(const basic_unicode_string& other, size_type pos)
      : basic_unicode_string(other, pos, other.size() - pos) {
  }  // Переполнение беззнакового числа допустимо

  basic_unicode_string(const basic_unicode_string& other,
                       size_type pos,
                       size_type count)
      : m_alc{allocator_traits_type::select_on_container_copy_construction(
            other.m_alc)} {
    become_small();
    copy_construct_by_pos_and_count(other, pos, count);
  }

  template <size_type N,
            class Allocator = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Allocator>, int> = 0>
  basic_unicode_string(const value_type* str,
                       size_type length,
                       Allocator&& alloc = Allocator{})
      : m_alc(forward<Allocator>(alloc)) {
    become_small();
    copy_construct_from_wide_str_unchecked(str, length);
  }

  template <class Allocator = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Allocator>, int> = 0>
  basic_unicode_string(const value_type* null_terminated_str,
                       Allocator&& alloc = Allocator{})
      : m_alc{forward<Allocator>(alloc)} {
    become_small();
    copy_construct_from_wide_str_unchecked(
        null_terminated_str,
        static_cast<size_type>(traits_type::length(null_terminated_str)));
  }

  template <class InputIt,
            class Allocator = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Allocator>, int> = 0>
  basic_unicode_string(InputIt first,
                       InputIt last,
                       Allocator&& alloc = Allocator{})
      : m_alc{forward<Allocator>(alloc)} {
    become_small();
    push_back_range_impl(
        first, last,
        is_same<typename iterator_traits<InputIt>::iterator_category,
                random_access_iterator_tag>{});
  }

  template <class Allocator = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Allocator>, int> = 0>
  basic_unicode_string(const native_str_t& nt_str,
                       Allocator&& alloc = Allocator{})
      : m_alc{forward<Allocator>(alloc)} {
    become_small();
    copy_construct_from_native_str_unchecked(nt_str);
  }

  basic_unicode_string(const basic_unicode_string& other)
      : basic_unicode_string(
            *other.raw_str(),
            allocator_traits_type::select_on_container_copy_construction(
                other.m_alc)) {}

  basic_unicode_string(basic_unicode_string&& other) noexcept
      : m_alc{move(other.m_alc)}, m_str{other.m_str} {
    if (other.is_small()) {
      m_str.Buffer = m_buffer;
      traits_type::copy(data(), other.data(), other.size());
    }
    other.become_small();
  }

  basic_unicode_string& assign(size_type count, value_type ch) {
    clear();
    resize(count, ch);
    return *this;
  }

  basic_unicode_string& assign(const native_str_t& nt_str) {
    if (raw_str() != addressof(nt_str)) {
      copy_construct_from_native_str_unchecked(nt_str);
    }
    return *this;
  }

  basic_unicode_string& assign(const basic_unicode_string& other) {
    return assign(*other.raw_str());  // Not a full equialent to *this = other
  }

  basic_unicode_string& assign(basic_unicode_string&& other) {
    return *this = move(other);
  }

  basic_unicode_string& assign(const basic_unicode_string& other,
                               size_type pos,
                               size_type count = npos) {
    copy_construct_by_pos_and_count(other, pos, count);
    return *this;
  }

  basic_unicode_string& assign(const value_type* str, size_type ch_count) {
    reserve(ch_count);
    traits_type::move(data(), str, ch_count);  // str может быть частью data()
    m_str.Length = ch_count_to_bytes(ch_count);
    return *this;
  }

  basic_unicode_string& assign(const value_type* null_terminated_str) {
    return assign(
        null_terminated_str,
        static_cast<size_type>(traits_type::length(null_terminated_str)));
  }

  template <class InputIt>
  basic_unicode_string& assign(InputIt first, InputIt last) {
    clear();
    push_back_range_impl(
        first, last,
        is_same<typename iterator_traits<InputIt>::iterator_category,
                random_access_iterator_tag>{});
    return *this;
  }

  basic_unicode_string& operator=(const basic_unicode_string& other) {
    using propagate_on_copy_assignment_t =
        typename allocator_traits_type::propagate_on_container_copy_assignment;

    if (this != addressof(other)) {
      copy_assignment_impl<propagate_on_copy_assignment_t::value>(other);
    }
    return *this;
  }

  basic_unicode_string& operator=(basic_unicode_string&& other) {
    using propagate_on_move_assignment_t =
        typename allocator_traits_type::propagate_on_container_move_assignment;
    constexpr bool move_alloc{
        is_same_v<typename allocator_traits_type::is_always_equal, true_type> ||
        propagate_on_move_assignment_t::value};

    if (this != addressof(other)) {
      move_assignment_impl<move_alloc>(
          move(other), typename allocator_traits_type::
                           propagate_on_container_move_assignment{});
    }
    return *this;
  }

  basic_unicode_string& operator=(const value_type* null_terminated_str) {
    assign(null_terminated_str);
    return *this;
  }

  basic_unicode_string& operator=(const native_str_t& nt_str) {
    assign(nt_str);
    return *this;
  }

  basic_unicode_string& operator=(value_type ch) {
    traits_type::assign(data(), 1, ch);
    m_str.Length = ch_count_to_bytes(1);
    return *this;
  }

  ~basic_unicode_string() noexcept { destroy(); }

  constexpr const allocator_type& get_allocator() const noexcept {
    return m_alc;
  }

  constexpr value_type& operator[](size_type idx) noexcept {
    return data()[idx];
  }

  constexpr const value_type& operator[](size_type idx) const noexcept {
    return data()[idx];
  }

  constexpr value_type& front() noexcept { return data()[0]; }

  constexpr const value_type& front() const noexcept { return data()[0]; }

  constexpr value_type& back() noexcept { return data()[size() - 1]; }

  constexpr const value_type& back() const noexcept {
    return data()[size() - 1];
  }

  constexpr value_type* data() noexcept { return m_str.Buffer; }

  constexpr const value_type* data() const noexcept { return m_str.Buffer; }

  constexpr native_str_t* raw_str() noexcept { return addressof(m_str); }

  constexpr const native_str_t* raw_str() const noexcept {
    return addressof(m_str);
  }

  iterator begin() noexcept { return data(); }

  const_iterator begin() const noexcept { return cbegin(); }

  iterator end() noexcept { return data() + size(); }

  const_iterator end() const noexcept { return cend(); }

  const_iterator cbegin() const noexcept { return data(); }

  const_iterator cend() const noexcept { return data() + size(); }

  constexpr bool empty() const noexcept { return size() == 0; }

  constexpr size_type size() const noexcept {
    return bytes_count_to_ch(m_str.Length);
  }

  constexpr size_type length() const noexcept {
    return bytes_count_to_ch(m_str.Length);
  }

  constexpr size_type max_size() const noexcept { return npos - 1; }

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

  constexpr size_type capacity() const noexcept {
    return bytes_count_to_ch(m_str.MaximumLength);
  }

  void resize(size_type count) { resize(count, value_type{}); }

  void resize(size_type count, value_type ch) {
    reserve(count);
    if (size() < count) {
      traits_type::assign(data(), count - size(), ch);
    }
    m_str.Length = ch_count_to_bytes(count);
  }

  void shrink_to_fit() {
    if (!is_small()) {
      const size_t current_size{size}, current_capacity{capacity()};
      if (current_capacity > current_size) {
        value_type *old_buffer{data()}, *new_buffer{nullptr};
        if (current_size <= SSO_BUFFER_CH_COUNT) {
          new_buffer = m_buffer;
        } else {
          new_buffer = allocate_buffer(m_alc, current_size);
        }
        traits_type::copy(new_buffer, old_buffer, current_size);
        m_str.Buffer = new_buffer;
        m_str.MaximumLength = current_size;
        deallocate_buffer(m_alc, old_buffer, current_capacity);
      }
    }
  }

  constexpr void clear() noexcept { m_str.Length = 0; }

  void push_back(value_type ch) {
    const size_type old_size{size()}, old_capacity{capacity()};
    if (old_capacity == old_size) {
      reserve(calc_growth(old_capacity));
    }
    traits_type::assign(data()[old_size], ch);
    m_str.Length += ch_count_to_bytes(1);
  }

  constexpr void pop_back() noexcept { m_str.Length -= ch_count_to_bytes(1); }

  size_type copy(value_type* dst, size_type count, size_type pos = 0) const {
    size_type copied{0};
    if (pos < size()) {
      const auto length{static_cast<size_type>(size() - pos)};
      copied = (min)(length, count);
      traits_type::move(dst, data() + pos, copied);
    }
    return copied;
  }

 private:
  constexpr void become_small(size_type size = 0) noexcept {
    m_str.Buffer = m_buffer;
    m_str.Length = ch_count_to_bytes(size);
    m_str.MaximumLength = SSO_BUFFER_CH_COUNT;
  }

  constexpr bool is_small() const noexcept {
    return capacity() <= SSO_BUFFER_CH_COUNT;
  }

  constexpr allocator_type& get_alloc() noexcept { return m_alc; }

  void copy_construct_from_wide_str_unchecked(const value_type* str,
                                              size_type count) {
    reserve(count);
    traits_type::copy(
        data(), str, count);  // Быстрее, чем move в assign(value_type*, size_t)
    m_str.Length = ch_count_to_bytes(count);
  }

  void copy_construct_from_native_str_unchecked(const native_str_t& nt_str) {
    copy_construct_from_wide_str_unchecked(nt_str.Buffer,
                                           bytes_count_to_ch(nt_str.Length));
  }

  void copy_construct_by_pos_and_count(const basic_unicode_string& other,
                                       size_type pos,
                                       size_type count) {
    if (pos < other.size()) {
      const auto length{static_cast<size_type>(other.size() - pos)};
      copy_construct_from_wide_str_unchecked(other.data() + pos,
                                             (min)(length, count));
    }
  }

  void construct_from_character_unchecked(size_type count, value_type ch) {
    reserve(count);
    traits_type::assign(data(), count, ch);
    m_str.Length = ch_count_to_bytes(count);
  }

  template <class InputIt>
  void push_back_range_impl(InputIt first, InputIt last, false_type) {
    for (; first != last; first = next(first)) {
      push_back(*first);
    }
  }

  template <class RandomAcceccIt>
  void push_back_range_impl(RandomAcceccIt first,
                            RandomAcceccIt last,
                            true_type) {
    size_type old_size{size()};
    const auto required_capacity{
        static_cast<size_type>(old_size + distance(first, last))};
    reserve(required_capacity);
    value_type* buffer{data() + old_size};
    for (; first != last; first = next(first), ++buffer) {
      traits_type::assign(*buffer, *first);
    }
    m_str.Length += ch_count_to_bytes(required_capacity);
  }

  template <bool CopyAlloc>
  void copy_assignment_impl(basic_unicode_string&& other) {
    destroy();
    if constexpr (CopyAlloc) {
      m_alc = other.m_alc;
    }
    copy_construct_from_native_str_unchecked(*other.raw_str());
  }

  template <bool AllocIsAlwaysEqual>
  void move_assignment_impl(basic_unicode_string&& other, false_type) {
    if constexpr (AllocIsAlwaysEqual) {
      move_assignment_impl<false>(move(other), true_type{});
    } else {
      copy_assignment_impl<false>(
          static_cast<const basic_unicode_string&>(other));
      other.destroy();
      other.become_small();
    }
  }

  template <bool MoveAlloc>
  void move_assignment_impl(basic_unicode_string&& other, true_type) {
    destroy();
    if (!other.is_small()) {
      m_str = other.m_str;
    } else {
      const size_t new_size{other.size()};
      become_small(new_size);
      traits_type::copy(data(), other.data(), new_size);
    }
    if constexpr (MoveAlloc) {
      m_alc = move(other.m_alc);
    }
    other.become_small();
  }

  void destroy() noexcept {
    if (!is_small()) {
      deallocate_buffer(get_alloc(), data(), capacity());
    }
  }

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