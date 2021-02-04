#pragma once
#include <new_delete.h>
#include <algorithm.hpp>
#include <allocator.hpp>
#include <char_traits_impl.hpp>
#include <string_fwd.hpp>
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
    concat(&fill_helper, count, ch);
  }

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc>
  basic_unicode_string(
      const basic_unicode_string<BufferSize, ChTraits, ChAlloc>& other,
      size_type pos)
      : basic_unicode_string(other, pos, other.size() - pos) {
  }  // Переполнение беззнакового числа допустимо

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc>
  basic_unicode_string(
      const basic_unicode_string<BufferSize, ChTraits, ChAlloc>& other,
      size_type pos,
      size_type count)
      : m_alc{allocator_traits_type::select_on_container_copy_construction(
            other.m_alc)} {
    become_small();
    append_by_pos_and_count(other, pos, count);
  }

  template <class Allocator = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Allocator>, int> = 0>
  basic_unicode_string(const value_type* str,
                       size_type length,
                       Allocator&& alloc = Allocator{})
      : m_alc(forward<Allocator>(alloc)) {
    become_small();
    concat(&copy_helper, length, str);
  }

  template <class Allocator = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Allocator>, int> = 0>
  basic_unicode_string(const value_type* null_terminated_str,
                       Allocator&& alloc = Allocator{})
      : m_alc{forward<Allocator>(alloc)} {
    become_small();
    concat(&copy_helper,
           static_cast<size_type>(traits_type::length(null_terminated_str)),
           null_terminated_str);
  }

  template <class InputIt,
            class Allocator = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Allocator>, int> = 0>
  basic_unicode_string(InputIt first,
                       InputIt last,
                       Allocator&& alloc = Allocator{})
      : m_alc{forward<Allocator>(alloc)} {
    become_small();
    append_range_impl<false_type>(
        first, last,
        is_same<typename iterator_traits<InputIt>::iterator_category,
                random_access_iterator_tag>{});
  }

  template <class Allocator = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Allocator>, int> = 0>
  basic_unicode_string(const native_unicode_str_t& nt_str,
                       Allocator&& alloc = Allocator{})
      : m_alc{forward<Allocator>(alloc)} {
    become_small();
    concat(&copy_helper, bytes_count_to_ch(nt_str.Length), nt_str.Buffer);
  }

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc,
            enable_if_t<BufferSize != SSO_BUFFER_CH_COUNT, int> = 0>
  basic_unicode_string(
      const basic_unicode_string<BufferSize, ChTraits, ChAlloc>& other)
      : basic_unicode_string(
            *other.raw_str(),
            allocator_traits_type::select_on_container_copy_construction(
                other.m_alc)) {}

  basic_unicode_string(const basic_unicode_string& other)
      : basic_unicode_string(
            *other.raw_str(),
            allocator_traits_type::select_on_container_copy_construction(
                other.m_alc)) {}

  basic_unicode_string(basic_unicode_string&& other) noexcept(
      is_nothrow_move_constructible_v<allocator_type>)
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

  basic_unicode_string& assign(const native_unicode_str_t& nt_str) {
    if (raw_str() != addressof(nt_str)) {
      clear();
      concat(&copy_helper, bytes_count_to_ch(nt_str.Length), nt_str.Buffer);
    }
    return *this;
  }

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc>
  basic_unicode_string& assign(
      const basic_unicode_string<BufferSize, ChTraits, ChAlloc>& other) {
    return assign(*other.raw_str());  // Not a full equialent to *this = other
  }

  basic_unicode_string& assign(basic_unicode_string&& other) {
    return *this = move(other);
  }

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc>
  basic_unicode_string& assign(
      const basic_unicode_string<BufferSize, ChTraits, ChAlloc>& other,
      size_type pos,
      size_type count = npos) {
    clear();
    append_by_pos_and_count(other, pos, count);
    return *this;
  }

  basic_unicode_string& assign(const value_type* str, size_type ch_count) {
    clear();
    concat(&move_helper, ch_count, str);
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
    append_range_impl<false_type>(
        first, last,
        is_same<typename iterator_traits<InputIt>::iterator_category,
                random_access_iterator_tag>{});
    return *this;
  }

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc,
            enable_if_t<BufferSize != SSO_BUFFER_CH_COUNT, int> = 0>
  basic_unicode_string& operator=(
      const basic_unicode_string<BufferSize, ChTraits, ChAlloc>& other) {
    using propagate_on_copy_assignment_t =
        typename allocator_traits_type::propagate_on_container_copy_assignment;

    if (this != addressof(other)) {
      copy_assignment_impl<propagate_on_copy_assignment_t::value>(other);
    }

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

  basic_unicode_string& operator=(basic_unicode_string&& other) noexcept(
      is_same_v<typename allocator_traits_type::is_always_equal, true_type> ||
      is_same_v<typename allocator_traits_type::
                    propagate_on_container_move_assignment,
                true_type> &&
          is_nothrow_move_assignable_v<allocator_type>) {
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

  basic_unicode_string& operator=(const native_unicode_str_t& nt_str) {
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

  value_type& at(size_type idx) {
    return at_index_verified(data(), size(), idx);
  }

  value_type& at(size_type idx) const {
    return at_index_verified(data(), size(), idx);
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

  constexpr native_unicode_str_t* raw_str() noexcept {
    return addressof(m_str);
  }

  constexpr const native_unicode_str_t* raw_str() const noexcept {
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

  constexpr size_type max_size() const noexcept {
    return bytes_count_to_ch(npos - 1);
  }

  void reserve(size_type new_capacity = 0) {
    if (new_capacity > size()) {
      grow_and_shift_unckecked(new_capacity);
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
      grow_and_shift_unckecked(calc_growth(old_capacity));
    }
    traits_type::assign(data()[old_size], ch);
    m_str.Length += ch_count_to_bytes(1);
  }

  constexpr void pop_back() noexcept { m_str.Length -= ch_count_to_bytes(1); }

  basic_unicode_string& append(size_type count, value_type ch) {
    concat_with_optimal_growth(&fill_helper, count, ch);
    return *this;
  }

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc>
  basic_unicode_string& append(
      const basic_unicode_string<BufferSize, ChTraits, ChAlloc>& str) {
    return append(*str.raw_str());
  }

  basic_unicode_string& append(const native_unicode_str_t& nt_str) {
    return append(nt_str.Buffer, bytes_count_to_ch(nt_str.Length));
  }

  basic_unicode_string& append(const value_type* str, size_type count) {
    concat_with_optimal_growth(&copy_helper, count, str);
    return *this;
  }

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc>
  basic_unicode_string& append(
      const basic_unicode_string<BufferSize, ChTraits, ChAlloc>& str,
      size_type pos,
      size_type count) {
    append_by_pos_and_count(str, pos, count);
    return *this;
  }

  basic_unicode_string& append(const value_type* null_terminated_str) {
    return append(null_terminated_str,
                  traits_type::length(null_terminated_str));
  }

  template <class InputIt>
  basic_unicode_string& append(InputIt first, InputIt last) {
    append_range_impl<true_type>(
        first, last,
        is_same<typename iterator_traits<InputIt>::iterator_category,
                random_access_iterator_tag>{});
    return *this;
  }

  basic_unicode_string& operator+=(value_type ch) {
    push_back(ch);
    return *this;
  }

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc>
  basic_unicode_string& operator+=(
      const basic_unicode_string<BufferSize, ChTraits, ChAlloc>& str) {
    return append(str);
  }

  basic_unicode_string& operator+=(const native_unicode_str_t& str) {
    return append(str);
  }

  basic_unicode_string& operator+=(const value_type* null_terminated_str) {
    return append(null_terminated_str);
  }

  // basic_unicode_string& insert(size_type index,
  //                             size_type count,
  //                             value_type ch) {
  //  if (index == size()) {
  //    return append(count, ch);
  //  }
  //}

  basic_unicode_string& insert(size_type index,
                               const value_type* str,
                               size_type count) {
    if (index == size()) {
      return append(str, count);
    }
  }

  basic_unicode_string& insert(size_type index,
                               const value_type* null_terminated_str) {
    return insert(index, null_terminated_str,
                  traits_type::length(null_terminated_str));
  }

  // basic_unicode_string& insert(size_type index,
  //                             const native_unicode_str_t& nt_str) {
  //  if (index == size()) {
  //    return append(nt_str);
  //  }
  //}

  // basic_unicode_string& insert(size_type index,
  //                             const basic_unicode_string& other) {
  //  if (index == size()) {
  //    return append(other);
  //  }
  //}

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

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc>
  void append_by_pos_and_count(
      const basic_unicode_string<BufferSize, ChTraits, ChAlloc>& other,
      size_type pos,
      size_type count) {
    if (pos < other.size()) {
      const auto length{static_cast<size_type>(other.size() - pos)};
      concat_with_optimal_growth(&copy_helper, (min)(length, count),
                                 other.data() + pos);
    }
  }

  template <typename GrowthTag, class InputIt>
  void append_range_impl(InputIt first, InputIt last, false_type) {
    for (; first != last; first = next(first)) {
      push_back(*first);
    }
  }

  template <typename OptimalGrowthTag, class RandomAcceccIt>
  void append_range_impl(RandomAcceccIt first, RandomAcceccIt last, true_type) {
    const auto range_length{static_cast<size_type>(distance(first, last))};
    append_range_by_random_access_iterators(first, last, range_length,
                                            OptimalGrowthTag{});
  }

  template <class RandomAcceccIt>
  void append_range_by_random_access_iterators(
      RandomAcceccIt first,
      [[maybe_unused]] RandomAcceccIt last,
      size_type range_length,
      false_type) {
    if constexpr (is_pointer_v<RandomAcceccIt>) {
      concat(&copy_helper, range_length, first);
    } else {
      concat(&copy_range_helper, range_length, first, last);
    }
  }

  template <class RandomAcceccIt>
  void append_range_by_random_access_iterators(
      RandomAcceccIt first,
      [[maybe_unused]] RandomAcceccIt last,
      size_type range_length,
      true_type) {
    if constexpr (is_pointer_v<RandomAcceccIt>) {
      concat_with_optimal_growth(&copy_helper, range_length, first);
    } else {
      concat_with_optimal_growth(&copy_range_helper, range_length, first, last);
    }
  }

  template <bool CopyAlloc,
            size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc>
  void copy_assignment_impl(
      const basic_unicode_string<BufferSize, ChTraits, ChAlloc>& other) {
    clear();
    destroy();
    if constexpr (CopyAlloc) {
      m_alc = other.m_alc;
    }
    concat(&copy_helper, other.size(), other.data());
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
      const size_type new_size{other.size()};
      become_small(new_size);
      traits_type::copy(data(), other.data(), new_size);
    }
    if constexpr (MoveAlloc) {
      m_alc = move(other.m_alc);
    }
    other.become_small();
  }

  template <class Handler, typename... Types>
  void concat_with_optimal_growth(Handler handler,
                                  size_type count,
                                  const Types&... args) {
    concat_impl(
        handler,
        []([[maybe_unused]] size_type old_cap, size_type required) {
          return required;
        },
        count, args...);
  }

  template <class Handler, typename... Types>
  void concat(Handler handler, size_type count, const Types&... args) {
    concat_impl(
        handler,
        []([[maybe_unused]] size_type old_cap, size_type required) {
          return calc_growth(old_cap, required);
        },
        count, args...);
  }

  template <class Handler, class CapacityCalculator, typename... Types>
  void concat_impl(Handler handler,
                   CapacityCalculator cap_calc,
                   size_type count,
                   const Types&... args) {
    size_type old_size{size()};
    reserve(cap_calc(capacity(), old_size + count));
    handler(data() + old_size, count, args...);
    m_str.Length += ch_count_to_bytes(count);
  }

  template <class Handler, typename... Types>
  void insert_impl(size_type index,
                   Handler handler,
                   size_type count,
                   const Types&... args) {
    const size_type old_size{size()};
    if (old_size + count >= capacity()) {
      // grow_and_shift_unckecked()
    }
  }

  void grow_and_shift_unckecked(size_type new_capacity,
                                pair<size_type, size_type> bounds = {}) {
    auto& alc{get_alloc()};
    value_type* old_buffer{data()};
    const size_type old_size{size()};
    value_type* new_buffer{new_capacity <= SSO_BUFFER_CH_COUNT
                               ? m_buffer
                               : allocate_buffer(alc, new_capacity)};
    shift_right_helper(&copy_helper, new_buffer, old_buffer, old_size, bounds);
    if (!is_small()) {
      deallocate_buffer(alc, old_buffer, capacity());
    }
    m_str.Buffer = new_buffer;
    m_str.Length = ch_count_to_bytes(old_size + (bounds.second - bounds.first));
    m_str.MaximumLength = ch_count_to_bytes(new_capacity);
  }

  void destroy() noexcept {
    if (!is_small()) {
      deallocate_buffer(get_alloc(), data(), capacity());
    }
  }

  template <class Buffer>
  static decltype(auto) at_index_verified(Buffer& buf,
                                          size_type size,
                                          size_type idx) {
    throw_exception_if_not<out_of_range>(idx < size, "index is out of range",
                                         constexpr_message_tag{});
    return buf[idx];
  }

  template <class Handler>
  static void shift_right_helper(Handler handler,
                                 value_type* dst,
                                 const value_type* src,
                                 size_type count,
                                 pair<size_type, size_type> bounds) {
    handler(dst, bounds.first, src);
    handler(dst + bounds.second, count - bounds.first, src + bounds.first);
  }

  static void copy_helper(value_type* dst,
                          size_type count,
                          const value_type* src) {
    traits_type::copy(dst, src, count);
  }

  template <class InputIt>
  static void copy_range_helper(value_type* dst, InputIt first, InputIt last) {
    for (; first != last; first = next(last), ++dst) {
      traits_type::assign(*dst, *first);
    }
  }

  static void move_helper(value_type* dst,
                          size_type count,
                          const value_type* src) {
    traits_type::move(dst, src, count);
  }

  static void fill_helper(value_type* dst,
                          size_type count,
                          value_type character) {
    traits_type::assign(dst, count, character);
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

  static constexpr size_type calc_growth(size_type old_capacity,
                                         size_type required) noexcept {
    return (max)(calc_growth(old_capacity), required);
  }

 private:
  allocator_type m_alc{};
  native_unicode_str_t m_str{};
  value_type m_buffer[SsoBufferChCount]{value_type{}};
};

namespace literals {
inline unicode_string operator""_us(const wchar_t* str, size_t length) {
  return unicode_string(str, static_cast<unicode_string::size_type>(length));
}

inline unicode_string_non_paged operator""_usnp(const wchar_t* str,
                                                size_t length) {
  return unicode_string_non_paged(
      str, static_cast<unicode_string::size_type>(length));
}
}  // namespace literals
}  // namespace ktl