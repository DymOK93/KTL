#pragma once
#include <exception.h>
#include <new_delete.h>
#include <algorithm.hpp>
#include <assert.hpp>
#include <iterator.hpp>
#include <limits.hpp>
#include <string_view.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

#include <ntddk.h>

namespace ktl::str::details {
template <size_t SsoBufferChCount,
          typename NativeStrTy,
          template <typename... CharT>
          class Traits,
          template <typename... CharT>
          class Alloc>
class basic_winnt_string {
 public:
  using native_string_type = NativeStrTy;
  using native_string_traits_type = native_string_traits<NativeStrTy>;
  using string_view_type = basic_winnt_string_view<native_string_type, Traits>;

  using value_type = typename native_string_traits_type::value_type;
  using size_type = typename native_string_traits_type::size_type;

  using traits_type = Traits<value_type>;

  using allocator_type = Alloc<value_type>;
  using allocator_traits_type = allocator_traits<allocator_type>;

  using difference_type = typename allocator_traits_type::difference_type;

  using reference = typename allocator_traits_type::reference;
  using const_reference = typename allocator_traits_type::const_reference;
  using pointer = typename allocator_traits_type::pointer;
  using const_pointer = typename allocator_traits_type::const_pointer;

  using iterator = pointer;
  using const_iterator = const_pointer;

 private:
  template <class Ty>
  struct not_convertible_to_native_str {
    static constexpr bool value =
        !is_convertible_v<const Ty&, const_pointer> &&
        !is_convertible_v<const Ty&, native_string_type>;
  };

 public:
  static constexpr size_type SSO_BUFFER_CH_COUNT{SsoBufferChCount};
  static constexpr auto npos{(numeric_limits<size_type>::max)()};

 private:
  static constexpr size_type GROWTH_MULTIPLIER{2};

 public:
  constexpr basic_winnt_string() noexcept { become_small(); }

  explicit constexpr basic_winnt_string(const allocator_type& alloc) noexcept(
      is_nothrow_copy_constructible_v<allocator_type>)
      : m_str{one_then_variadic_args{}, alloc} {
    become_small();
  }

  explicit constexpr basic_winnt_string(allocator_type&& alloc) noexcept(
      is_nothrow_move_constructible_v<allocator_type>)
      : m_str{one_then_variadic_args{}, move(alloc)} {
    become_small();
  }

  template <class Allocator = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Allocator>, int> = 0>
  basic_winnt_string(size_type count,
                     value_type ch,
                     Allocator&& alloc = Allocator{})
      : m_str{one_then_variadic_args{}, forward<Allocator>(alloc)} {
    become_small();
    concat_without_transfer_old_data(make_fill_helper(), count, ch);
  }

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc>
  basic_winnt_string(const basic_winnt_string<BufferSize,
                                              native_string_type,
                                              ChTraits,
                                              ChAlloc>& other,
                     size_type pos)
      : basic_winnt_string(other, pos, other.size() - pos) {
  }  // Переполнение беззнакового числа допустимо

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc>
  basic_winnt_string(const basic_winnt_string<BufferSize,
                                              native_string_type,
                                              ChTraits,
                                              ChAlloc>& other,
                     size_type pos,
                     size_type count)
      : m_str{one_then_variadic_args{},
              allocator_traits_type::select_on_container_copy_construction(
                  other.get_alloc())} {
    become_small();
    append_by_pos_and_count<false>(other, pos, count);
  }

  template <class Ty,
            class Allocator = allocator_type,
            enable_if_t<is_convertible_v<Ty, string_view_type> &&
                            not_convertible_to_native_str<Ty>::value,
                        int> = 0>
  basic_winnt_string(const Ty& value, Allocator&& alloc = Allocator{})
      : basic_winnt_string(*static_cast<string_view_type>(value).raw_str(),
                           forward<Allocator>(alloc)) {}

  template <class Allocator = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Allocator>, int> = 0>
  basic_winnt_string(const value_type* str,
                     size_type length,
                     Allocator&& alloc = Allocator{})
      : m_str{one_then_variadic_args{}, forward<Allocator>(alloc)} {
    become_small();
    concat_without_transfer_old_data(make_copy_helper(), length, str);
  }

  template <class Allocator = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Allocator>, int> = 0>
  basic_winnt_string(const value_type* null_terminated_str,
                     Allocator&& alloc = Allocator{})
      : m_str{one_then_variadic_args{}, forward<Allocator>(alloc)} {
    become_small();
    concat_without_transfer_old_data(
        make_copy_helper(),
        static_cast<size_type>(traits_type::length(null_terminated_str)),
        null_terminated_str);
  }

  template <class InputIt,
            class Allocator = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Allocator>, int> = 0>
  basic_winnt_string(InputIt first,
                     InputIt last,
                     Allocator&& alloc = Allocator{})
      : m_str{one_then_variadic_args{}, forward<Allocator>(alloc)} {
    become_small();
    append_range_impl<false_type>(
        first, last,
        is_same<typename iterator_traits<InputIt>::iterator_category,
                random_access_iterator_tag>{});
  }

  template <class Allocator = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Allocator>, int> = 0>
  basic_winnt_string(native_string_type native_str,
                     Allocator&& alloc = Allocator{})
      : m_str{one_then_variadic_args{}, forward<Allocator>(alloc)} {
    become_small();
    concat_without_transfer_old_data(
        make_copy_helper(), native_string_traits_type::get_size(native_str),
        native_str.Buffer);
  }

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc,
            enable_if_t<BufferSize != SSO_BUFFER_CH_COUNT, int> = 0>
  basic_winnt_string(const basic_winnt_string<BufferSize,
                                              native_string_type,
                                              ChTraits,
                                              ChAlloc>& other)
      : basic_winnt_string(
            *other.raw_str(),
            allocator_traits_type::select_on_container_copy_construction(
                other.get_alloc())) {}

  basic_winnt_string(const basic_winnt_string& other)
      : basic_winnt_string(
            *other.raw_str(),
            allocator_traits_type::select_on_container_copy_construction(
                other.get_alloc())) {}

  basic_winnt_string(basic_winnt_string&& other) noexcept(
      is_nothrow_move_constructible_v<allocator_type>)
      : m_str{move(other.m_str)} {
    if (other.is_small()) {
      native_string_traits_type::set_buffer(get_native_str(), m_buffer);
      traits_type::copy(data(), other.data(), other.size());
    }
    other.become_small();
  }

  basic_winnt_string& assign(size_type count, value_type ch) {
    clear();
    resize(count, ch);
    return *this;
  }

  basic_winnt_string& assign(native_string_type native_str) {
    if (raw_str() != addressof(native_str)) {
      clear();
      concat_without_transfer_old_data(
          make_copy_helper(), native_string_traits_type::get_size(native_str),
          native_str.Buffer);
    }
    return *this;
  }

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc>
  basic_winnt_string& assign(const basic_winnt_string<BufferSize,
                                                      native_string_type,
                                                      ChTraits,
                                                      ChAlloc>& other) {
    return assign(*other.raw_str());  // Not a full equialent to *this =
    other
  }

  basic_winnt_string& assign(basic_winnt_string&& other) {
    return *this = move(other);
  }

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc>
  basic_winnt_string& assign(const basic_winnt_string<BufferSize,
                                                      native_string_type,
                                                      ChTraits,
                                                      ChAlloc>& other,
                             size_type pos,
                             size_type count = npos) {
    clear();
    append_by_pos_and_count<false>(other, pos, count);
    return *this;
  }

  basic_winnt_string& assign(const value_type* str, size_type ch_count) {
    clear();
    concat_without_transfer_old_data(make_move_helper(), ch_count, str);
    return *this;
  }

  basic_winnt_string& assign(const value_type* null_terminated_str) {
    return assign(
        null_terminated_str,
        static_cast<size_type>(traits_type::length(null_terminated_str)));
  }

  template <class InputIt>
  basic_winnt_string& assign(InputIt first, InputIt last) {
    clear();
    append_range_impl<false_type>(
        first, last,
        is_same<typename iterator_traits<InputIt>::iterator_category,
                random_access_iterator_tag>{});
    return *this;
  }

  template <class Ty,
            class Allocator = allocator_type,
            enable_if_t<is_convertible_v<Ty, string_view_type> &&
                            not_convertible_to_native_str<Ty>::value,
                        int> = 0>
  basic_winnt_string& assign(const Ty& value) {
    return assign(*static_cast<string_view_type>(value).raw_str());
  }

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc,
            enable_if_t<BufferSize != SSO_BUFFER_CH_COUNT, int> = 0>
  basic_winnt_string& operator=(const basic_winnt_string<BufferSize,
                                                         native_string_type,
                                                         ChTraits,
                                                         ChAlloc>& other) {
    using propagate_on_copy_assignment_t =
        typename allocator_traits_type::propagate_on_container_copy_assignment;

    if (this != addressof(other)) {
      copy_assignment_impl<>(other, propagate_on_copy_assignment_t{});
    }

    return *this;
  }

  basic_winnt_string& operator=(const basic_winnt_string& other) {
    using propagate_on_copy_assignment_t =
        typename allocator_traits_type::propagate_on_container_copy_assignment;

    if (this != addressof(other)) {
      copy_assignment_impl(other, propagate_on_copy_assignment_t{});
    }

    return *this;
  }

  basic_winnt_string& operator=(basic_winnt_string&& other) noexcept(
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

  basic_winnt_string& operator=(const value_type* null_terminated_str) {
    assign(null_terminated_str);
    return *this;
  }

  basic_winnt_string& operator=(native_string_type native_str) {
    assign(native_str);
    return *this;
  }

  basic_winnt_string& operator=(value_type ch) {
    traits_type::assign(data(), 1, ch);
    native_string_traits_type::set_size(get_native_str(), 1);
    return *this;
  }

  template <class Ty,
            class Allocator = allocator_type,
            enable_if_t<is_convertible_v<Ty, string_view_type> &&
                            not_convertible_to_native_str<Ty>::value,
                        int> = 0>
  basic_winnt_string& operator=(const Ty& value) {
    return *this = *static_cast<string_view_type>(value).raw_str();
  }

  ~basic_winnt_string() noexcept { destroy(); }

  constexpr const allocator_type& get_allocator() const noexcept {
    return get_alloc();
  }

  value_type& at(size_type idx) {
    return at_index_verified(data(), size(), idx);
  }

  value_type& at(size_type idx) const {
    return at_index_verified(data(), size(), idx);
  }

  constexpr value_type& operator[](size_type idx) noexcept {
    assert_with_msg(idx < size(), L"index is out of range");
    return data()[idx];
  }

  constexpr const value_type& operator[](size_type idx) const noexcept {
    assert_with_msg(idx < size(), L"index is out of range");
    return data()[idx];
  }

  constexpr value_type& front() noexcept {
    assert_with_msg(!empty(), L"front() called on empty string");
    return data()[0];
  }

  constexpr const value_type& front() const noexcept {
    assert_with_msg(!empty(), L"front() called on empty string");
    return data()[0];
  }

  constexpr value_type& back() noexcept {
    assert_with_msg(!empty(), L"back() called at empty string");
    return data()[size() - 1];
  }

  constexpr const value_type& back() const noexcept {
    assert_with_msg(!empty(), L"front() called at empty string");
    return data()[size() - 1];
  }

  constexpr value_type* data() noexcept {
    return native_string_traits_type::get_buffer(get_native_str());
  }

  constexpr const value_type* data() const noexcept {
    return native_string_traits_type::get_buffer(get_native_str());
  }

  constexpr operator string_view_type() const noexcept { return {*raw_str()}; }

  constexpr native_string_type* raw_str() noexcept {
    return addressof(get_native_str());
  }

  constexpr const native_string_type* raw_str() const noexcept {
    return addressof(get_native_str());
  }

  iterator begin() noexcept { return data(); }

  const_iterator begin() const noexcept { return cbegin(); }

  iterator end() noexcept { return data() + size(); }

  const_iterator end() const noexcept { return cend(); }

  const_iterator cbegin() const noexcept { return data(); }

  const_iterator cend() const noexcept { return data() + size(); }

  constexpr bool empty() const noexcept { return size() == 0; }

  constexpr size_type size() const noexcept {
    return native_string_traits_type::get_size(get_native_str());
  }

  constexpr size_type length() const noexcept { return size(); }

  constexpr size_type max_size() const noexcept {
    return native_string_traits_type::get_max_size();
  }

  void reserve(size_type new_capacity = 0) {
    grow_if_needed(new_capacity, &transfer_without_shift);
  }

  constexpr size_type capacity() const noexcept {
    return native_string_traits_type::get_capacity(get_native_str());
  }

  void shrink_to_fit() {
    if (!is_small()) {
      const size_t current_size{size}, current_capacity{capacity()};
      if (current_capacity > current_size) {
        value_type *old_buffer{data()}, *new_buffer{nullptr};
        auto& alloc{get_alloc()};
        if (current_size <= SSO_BUFFER_CH_COUNT) {
          new_buffer = m_buffer;
        } else {
          new_buffer = allocate_buffer(alloc, current_size);
        }
        traits_type::copy(new_buffer, old_buffer, current_size);
        native_string_traits_type::set_buffer(get_native_str(), m_str,
                                              new_buffer);
        native_string_traits_type::set_capacity(get_native_str(), m_str,
                                                current_size);
        deallocate_buffer(alloc, old_buffer, current_capacity);
      }
    }
  }

  constexpr void clear() noexcept {
    native_string_traits_type::set_size(get_native_str(), 0);
  }

  void push_back(value_type ch) {
    const size_type old_size{size()}, old_capacity{capacity()};
    if (old_capacity == old_size) {
      grow(calc_growth(), &transfer_without_shift);
    }
    traits_type::assign(data()[old_size], ch);
    native_string_traits_type::increase_size(get_native_str(), 1);
  }

  constexpr void pop_back() noexcept {
    native_string_traits_type::decrease_size(get_native_str(), m_str, 1);
  }

  basic_winnt_string& append(size_type count, value_type ch) {
    concat_with_optimal_growth(make_fill_helper(), count, ch);
    return *this;
  }

  // traits_type must be equivalent
  template <size_t BufferSize, template <typename... CharType> class ChAlloc>
  basic_winnt_string& append(
      const basic_winnt_string<BufferSize, native_string_type, Traits, ChAlloc>&
          str) {
    return append(*str.raw_str());
  }

  basic_winnt_string& append(native_string_type native_str) {
    return append(native_str.Buffer,
                  native_string_traits_type::get_size(native_str));
  }

  basic_winnt_string& append(const value_type* str, size_type count) {
    concat_with_optimal_growth(make_copy_helper(), count, str);
    return *this;
  }

  // traits_type must be equivalent
  template <size_t BufferSize, template <typename... CharType> class ChAlloc>
  basic_winnt_string& append(
      const basic_winnt_string<BufferSize, native_string_type, Traits, ChAlloc>&
          str,
      size_type pos,
      size_type count) {
    append_by_pos_and_count<true>(str, pos, count);
    return *this;
  }

  basic_winnt_string& append(const value_type* null_terminated_str) {
    return append(
        null_terminated_str,
        static_cast<size_type>(traits_type::length(null_terminated_str)));
  }

  template <class InputIt>
  basic_winnt_string& append(InputIt first, InputIt last) {
    append_range_impl<true_type>(
        first, last,
        is_same<typename iterator_traits<InputIt>::iterator_category,
                random_access_iterator_tag>{});
    return *this;
  }

  template <class Ty,
            class Allocator = allocator_type,
            enable_if_t<is_convertible_v<Ty, string_view_type> &&
                            not_convertible_to_native_str<Ty>::value,
                        int> = 0>
  basic_winnt_string& append(const Ty& value) {
    return append(*static_cast<string_view_type>(value).raw_str());
  }

  basic_winnt_string& operator+=(value_type ch) {
    push_back(ch);
    return *this;
  }

  // traits_type must be equivalent
  template <size_t BufferSize, template <typename... CharType> class ChAlloc>
  basic_winnt_string& operator+=(
      const basic_winnt_string<BufferSize, native_string_type, Traits, ChAlloc>&
          str) {
    return append(str);
  }

  basic_winnt_string& operator+=(native_string_type str) { return append(str); }

  basic_winnt_string& operator+=(const value_type* null_terminated_str) {
    return append(null_terminated_str);
  }

  template <class Ty,
            class Allocator = allocator_type,
            enable_if_t<is_convertible_v<Ty, string_view_type> &&
                            not_convertible_to_native_str<Ty>::value,
                        int> = 0>
  basic_winnt_string& operator+=(const Ty& value) {
    return *this += *static_cast<string_view_type>(value).raw_str();
  }

  template <size_t BufferSize, template <typename... CharType> class ChAlloc>
  [[nodiscard]] constexpr int compare(
      const basic_winnt_string<BufferSize, native_string_type, Traits, ChAlloc>&
          other) const noexcept {
    return compare_unchecked(data(), size(), other.data(), other.size());
  }

  [[nodiscard]] constexpr int compare(size_type my_pos,
                                      size_type my_count,
                                      native_string_type native_str) const {
    const size_type current_size{size()};
    throw_exception_if_not(my_pos < current_size);
    return native_string_traits_type::compare_unchecked(
        data() + my_pos, (min)(my_count, current_size - my_pos),
        native_string_traits_type::get_buffer(native_str),
        native_string_traits_type::get_size(native_str));
  }

  template <size_t BufferSize, template <typename... CharType> class ChAlloc>
  [[nodiscard]] constexpr int compare(
      size_type my_pos,
      size_type my_count,
      const basic_winnt_string<BufferSize, native_string_type, Traits, ChAlloc>&
          other) const {
    return compare(my_pos, my_count, other.raw_str());
  }

  [[nodiscard]] constexpr int compare(size_type my_pos,
                                      size_type my_count,
                                      native_string_type native_str,
                                      size_type other_pos,
                                      size_type other_count = npos) const {
    const size_type other_size{native_string_traits_type::get_size(native_str)};
    throw_exception_if_not(other_pos < other_size);
    return native_string_traits_type::compare_unchecked(
        data(), my_pos, my_count,
        native_string_traits_type::get_buffer(native_str) + other_pos,
        (min)(other_size - other_pos, other_count));
  }

  template <size_t BufferSize, template <typename... CharType> class ChAlloc>
  [[nodiscard]] constexpr int compare(
      size_type my_pos,
      size_type my_count,
      const basic_winnt_string<BufferSize, native_string_type, Traits, ChAlloc>&
          other,
      size_type other_pos,
      size_type other_count = npos) const {
    return compare(my_pos, my_count, *other.raw_str(), other_pos, other_count);
  }

  [[nodiscard]] constexpr int compare(
      const value_type* null_terminated_str) const {
    return compare(0, size(), null_terminated_str);
  }

  [[nodiscard]] constexpr int compare(
      size_type my_pos,
      size_type my_count,
      const value_type* null_terminated_str) const {
    return compare(
        my_pos, my_count, null_terminated_str,
        static_cast<size_type>(traits_type::length(null_terminated_str)));
  }

  [[nodiscard]] constexpr int compare(size_type my_pos,
                                      size_type my_count,
                                      const value_type* str,
                                      size_type other_count) const {
    const size_type current_size{size()};
    throw_exception_if_not(my_pos < current_size);
    const value_type* substr{data() + my_pos};
    const size_type substr_length{(min)(current_size - my_pos, my_count)};
    return native_string_traits_type::compare_unchecked(
        substr, str, substr_length, other_count);
  }

  [[nodiscard]] constexpr int compare(
      native_string_type native_str) const noexcept {
    return compare_unchecked(data(), size(),
                             native_string_traits_type::get_buffer(native_str),
                             native_string_traits_type::get_size(native_str));
  }

  [[nodiscard]] constexpr int compare(native_string_type native_str,
                                      size_type other_pos,
                                      size_type other_count) const noexcept {
    const string_view_type view{native_str};
    const auto view_size{static_cast<size_type>(view.size())};
    throw_out_of_range_if_not(other_pos < view_size);
    return native_string_traits_type::compare_unchecked(
        data(), size(), view.data(), (min)(view_size - other_pos, other_count));
  }

  template <class Ty,
            class Allocator = allocator_type,
            enable_if_t<is_convertible_v<Ty, string_view_type> &&
                            not_convertible_to_native_str<Ty>::value,
                        int> = 0>
  int compare(const Ty& value) noexcept(
      is_nothrow_convertible_v<Ty, string_view_type>) {
    return compare(*static_cast<string_view_type>(value).raw_str());
  }

  template <class Ty,
            class Allocator = allocator_type,
            enable_if_t<is_convertible_v<Ty, string_view_type> &&
                            not_convertible_to_native_str<Ty>::value,
                        int> = 0>
  int compare(size_type my_pos, size_type my_count, const Ty& value) noexcept(
      is_nothrow_convertible_v<Ty, string_view_type>) {
    return compare(my_pos, my_count,
                   *static_cast<string_view_type>(value).raw_str());
  }

  template <class Ty,
            class Allocator = allocator_type,
            enable_if_t<is_convertible_v<Ty, string_view_type> &&
                            not_convertible_to_native_str<Ty>::value,
                        int> = 0>
  int compare(
      size_type my_pos,
      size_type my_count,
      const Ty& value,
      size_type other_pos,
      size_type
          other_count) noexcept(is_nothrow_convertible_v<Ty,
                                                         string_view_type<
                                                             native_string_type,
                                                             Traits>>) {
    return compare(my_pos, my_count,
                   *static_cast<string_view_type>(value).raw_str(), other_pos,
                   other_count);
  }

  basic_winnt_string& insert(size_type index, size_type count, value_type ch) {
    throw_out_of_range_if_not(index > size());
    insert(begin() + index, count, ch);
    return *this;
  }

  basic_winnt_string& insert(size_type index,
                             const value_type* str,
                             size_type count) {
    const size_type current_size{size()};
    throw_out_of_range_if_not(index <= current_size);
    if (index == current_size) {
      return append(str, count);
    }
    insert_impl(index, make_copy_helper(), count, str);
    return *this;
  }

  basic_winnt_string& insert(size_type index,
                             const value_type* null_terminated_str) {
    return insert(
        index, null_terminated_str,
        static_cast<size_type>(traits_type::length(null_terminated_str)));
  }

  basic_winnt_string& insert(size_type index, native_string_type native_str) {
    return insert(index, native_string_traits_type::get_buffer(native_str),
                  native_string_traits_type::get_size(native_str));
  }

  template <size_t BufferSize, template <typename... CharType> class ChAlloc>
  basic_winnt_string& insert(
      size_type index,
      const basic_winnt_string<BufferSize, native_string_type, Traits, ChAlloc>&
          other) {
    return insert(*other.raw_str());
  }

  template <size_t BufferSize, template <typename... CharType> class ChAlloc>
  basic_winnt_string& insert(
      size_type index,
      const basic_winnt_string<BufferSize, native_string_type, Traits, ChAlloc>&
          other,
      size_type other_pos,
      size_type other_count) {
    const size_type other_size{other.size()};
    throw_out_of_range_if_not(index <= current_size);
    if (index == size()) {
      return append(str, pos, count);
    }
    throw_out_of_range_if_not(other_pos <= other_size);
    insert_impl(index, make_copy_helper(), (min)(other_size - other_pos, count),
                other.data() + other_pos);
    return *this;
  }

  iterator insert(const_iterator pos, value_type ch) {
    push_back(ch);
    return this;
  }

  iterator insert(const_iterator pos, size_type count, value_type ch) {
    if (pos == end()) {
      append(pos, count, ch);
    }
    insert_impl(index, make_fill_helper(), count, ch);
  }

  template <class InputIt>
  iterator insert(const_iterator pos, InputIt first, InputIt last) {
    if (pos == end()) {
      append(first, last);
    }
    insert_range_impl(
        pos - begin(), first, last,
        is_same<typename iterator_traits<InputIt>::iterator_category,
                random_access_iterator_tag>{});
  }

  template <class Ty,
            class Allocator = allocator_type,
            enable_if_t<is_convertible_v<Ty, string_view_type> &&
                            not_convertible_to_native_str<Ty>::value,
                        int> = 0>
  basic_winnt_string& insert(size_type index, const Ty& value) {
    return insert(index, *static_cast<string_view_type>(value).raw_str());
  }

  template <class Ty,
            class Allocator = allocator_type,
            enable_if_t<is_convertible_v<Ty, string_view_type> &&
                            not_convertible_to_native_str<Ty>::value,
                        int> = 0>
  basic_winnt_string& insert(size_type index,
                             const Ty& value,
                             size_type sv_pos,
                             size_type sv_count) {
    const auto view{static_cast<string_view_type>(value)};
    return insert(index, *view.substr(sv_pos, sv_count).raw_str());
  }

  basic_winnt_string& erase(size_type index = 0, size_type count = npos) {
    const size_type old_size{size()};
    throw_out_of_range_if_not(index <= old_size);
    erase_unchecked(index,
                    (min)(count, static_cast<size_type>(old_size - index)));
    return *this;
  }

  iterator erase(const_iterator pos) noexcept {
    const auto index{static_cast<size_type>(pos - begin())};
    assert(index < size());
    erase_unchecked(index, 1);
    return begin() + index;
  }

  iterator erase(const_iterator first, const_iterator last) noexcept {
    const auto index{static_cast<size_type>(first - begin())},
        range_length{static_cast<size_type>(last - first)};
    assert(index + range_length <= size());
    erase_unchecked(index, range_length);
    return begin() + index;
  }

  constexpr bool starts_with(value_type ch) const noexcept {
    return static_cast<string_view_type>(*this).starts_with(ch);
  }

  constexpr bool starts_with(const value_type* str,
                             size_type length) const noexcept {
    return static_cast<string_view_type>(*this).starts_with(str, length);
  }

  constexpr bool starts_with(
      const value_type* null_terminated_str) const noexcept {
    return static_cast<string_view_type>(*this).starts_with(
        null_terminated_str);
  }

  constexpr bool ends_with(value_type ch) const noexcept {
    return static_cast<string_view_type>(*this).ends_with(ch);
  }

  constexpr bool ends_with(const value_type* str,
                           size_type length) const noexcept {
    return static_cast<string_view_type>(*this).ends_with(str, length);
  }

  constexpr bool ends_with(
      const value_type* null_terminated_str) const noexcept {
    return static_cast<string_view_type>(*this).ends_with(null_terminated_str);
  }

  template <size_t BufferSize, template <typename... CharType> class ChAlloc>
  constexpr bool contains(
      const basic_winnt_string<BufferSize, native_string_type, Traits, ChAlloc>&
          other) const noexcept {
    return find(other, 0) != npos;
  }

  constexpr bool contains(native_string_type native_str) const noexcept {
    return find(native_str) != npos;
  }

  constexpr bool contains(
      const value_type* null_terminated_str) const noexcept {
    return find(null_terminated_str) != npos;
  }

  constexpr bool contains(value_type ch) const noexcept {
    return find(ch) != npos;
  }

  template <class Ty,
            class Allocator = allocator_type,
            enable_if_t<is_convertible_v<Ty, string_view_type> &&
                            not_convertible_to_native_str<Ty>::value,
                        int> = 0>
  constexpr bool contains(const Ty& value) const noexcept {
    return contains(*static_cast<string_view_type>(value).raw_str());
  }

  [[nodiscard]] basic_winnt_string substr(size_type pos = 0,
                                          size_type count = npos) {
    size_type current_size{size()};
    throw_out_of_range_if_not(pos < current_size);
    return basic_winnt_string{
        data() + pos, (min)(static_cast<size_type>(current_size - pos), count)};
  }

  size_type copy(value_type* dst, size_type count, size_type pos = 0) const {
    const size_type current_size{size()};
    throw_out_of_range_if_not(pos < current_size);
    const auto copied{(min)(static_cast<size_type>(current_size - pos), count)};
    traits_type::copy(dst, data() + pos,
                      copied);  // Пользователь отвечает за то, что диапазоны не
                                // должны пересекаться
    return copied;
  }

  void resize(size_type new_size) { resize(new_size, value_type{}); }

  void resize(size_type new_size, value_type ch) {
    reserve(new_size);
    if (size() < new_size) {
      traits_type::assign(data(), new_size - size(), ch);
    }
    native_string_traits_type::set_size(get_native_str(), new_size);
  }

  void swap(basic_winnt_string& other) { ::swap(*this, other); }

  template <size_t BufferSize, template <typename... CharType> class ChAlloc>
  constexpr size_type find(
      const basic_winnt_string<BufferSize, native_string_type, Traits, ChAlloc>&
          other,
      size_type my_pos) const noexcept {
    const size_type current_size{size()};
    if (my_pos > current_size) {
      return npos;
    }
    const auto first{begin()}, last{end()};
    const auto found_pos{find_subrange(first + my_pos, last, other.begin(),
                                       other.end(), make_ch_comparator()) -
                         first};
    return found_pos == current_size ? npos : static_cast<size_type>(found_pos);
  }

  constexpr size_type find(const value_type* null_terminated_str,
                           size_type my_pos,
                           size_type str_count) const noexcept {
    return static_cast<string_view_type>(*this).find(null_terminated_str,
                                                     my_pos, str_count)
  }

  constexpr size_type find(
      const value_type* null_terminated_str) const noexcept {
    return static_cast<string_view_type>(*this).find(null_terminated_str)
  }

  constexpr size_type find(value_type ch, size_type pos = 0) const noexcept {
    const size_type current_size{size()};
    if (pos >= current_size) {
      return npos;
    }
    const auto first{data() + pos};
    const auto found_pos{traits_type::find(first, current_size - pos, ch) -
                         first};
    return found_pos == size() ? npos : static_cast<size_type>(found_pos);
  }

  template <size_t BufferSize, template <typename... CharType> class ChAlloc>
  constexpr size_type find_first_of(
      const basic_winnt_string<BufferSize, native_string_type, Traits, ChAlloc>&
          other,
      size_type my_pos) const noexcept {
    return find(other, my_pos);
  }

  constexpr size_type find_first_of(const value_type* null_terminated_str,
                                    size_type str_pos,
                                    size_type str_count) const noexcept {
    return find(null_terminated_str, str_pos, str_count);
  }

  constexpr size_type find_first_of(
      const value_type* null_terminated_str) const noexcept {
    return find(null_terminated_str);
  }

  constexpr size_type find_first_of(value_type ch,
                                    size_type pos = 0) const noexcept {
    return find(ch, pos);
  }

  constexpr size_type find_first_not_of(value_type ch,
                                        size_type pos = 0) const noexcept {
    return find_if(begin(), end(), [ch](value_type val) { return val != ch; });
  }

 private:
  constexpr void become_small(size_type size = 0) noexcept {
    native_string_traits_type::set_buffer(get_native_str(), m_buffer);
    native_string_traits_type::set_size(get_native_str(), size);
    native_string_traits_type::set_capacity(get_native_str(),
                                            SSO_BUFFER_CH_COUNT);
  }

  constexpr bool is_small() const noexcept {
    return capacity() <= SSO_BUFFER_CH_COUNT;
  }

  constexpr allocator_type& get_alloc() noexcept { return m_str.get_first(); }

  constexpr const allocator_type& get_alloc() const noexcept {
    return m_str.get_first();
  }

  constexpr native_string_type& get_native_str() noexcept {
    return m_str.get_second();
  }

  constexpr const native_string_type& get_native_str() const noexcept {
    return m_str.get_second();
  }

  template <bool CalcOptimalGrowth,
            size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc>
  void append_by_pos_and_count(const basic_winnt_string<BufferSize,
                                                        native_string_type,
                                                        ChTraits,
                                                        ChAlloc>& other,
                               size_type pos,
                               size_type count) {
    const size_type other_size{other.size()};
    throw_out_of_range_if_not(pos < other_size);
    const auto length{static_cast<size_type>(other_size - pos)};
    if constexpr (CalcOptimalGrowth) {
      concat_with_optimal_growth(make_copy_helper(), (min)(length, count),
                                 other.data() + pos);
    } else {
      concat_without_transfer_old_data(make_copy_helper(), (min)(length, count),
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
      concat(make_copy_helper(), range_length, first);
    } else {
      concat(make_copy_range_helper(), range_length, first, last);
    }
  }

  template <class InputIt>
  void insert_range_impl(size_type index,
                         InputIt first,
                         InputIt last,
                         false_type) {
    const basic_winnt_string proxy{first, last};
    insert(index, proxy);
  }

  template <class RandomAcceccIt>
  void insert_range_impl(size_type index,
                         RandomAcceccIt first,
                         RandomAcceccIt last,
                         true_type) {
    const auto range_length{static_cast<size_type>(distance(first, last))};
    insert_range_by_random_access_iterators(first, last, range_length);
  }

  template <class RandomAcceccIt>
  void insert_range_by_random_access_iterators(
      size_type index,
      RandomAcceccIt first,
      [[maybe_unused]] RandomAcceccIt last,
      size_type range_length,
      false_type) {
    if constexpr (is_pointer_v<RandomAcceccIt>) {
      insert_impl(index, make_copy_helper(), range_length, first);
    } else {
      insert_impl(index, make_copy_range_helper(), range_length, first, last);
    }
  }

  template <class RandomAcceccIt>
  void append_range_by_random_access_iterators(
      RandomAcceccIt first,
      [[maybe_unused]] RandomAcceccIt last,
      size_type range_length,
      true_type) {
    if constexpr (is_pointer_v<RandomAcceccIt>) {
      concat_with_optimal_growth(make_copy_helper(), range_length, first);
    } else {
      concat_with_optimal_growth(make_copy_range_helper(), range_length, first,
                                 last);
    }
  }

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc>
  void copy_assignment_impl(const basic_winnt_string<BufferSize,
                                                     native_string_type,
                                                     ChTraits,
                                                     ChAlloc>& other,
                            true_type) {
    destroy();
    become_small();
    get_alloc() = other.get_alloc();
    concat_without_transfer_old_data(make_copy_helper(), other.size(),
                                     other.data());
  }

  template <size_t BufferSize,
            template <typename... CharType>
            class ChTraits,
            template <typename... CharType>
            class ChAlloc>
  void copy_assignment_impl(const basic_winnt_string<BufferSize,
                                                     native_string_type,
                                                     ChTraits,
                                                     ChAlloc>& other,
                            false_type) {
    clear();
    concat_without_transfer_old_data(make_copy_helper(), other.size(),
                                     other.data());
  }

  template <bool AllocIsAlwaysEqual>
  void move_assignment_impl(basic_winnt_string&& other, false_type) {
    if constexpr (AllocIsAlwaysEqual) {
      move_assignment_impl<false>(move(other), true_type{});
    } else {
      copy_assignment_impl(static_cast<const basic_winnt_string&>(other),
                           false_type{});
      other.destroy();
      other.become_small();
    }
  }

  template <bool MoveAlloc>
  void move_assignment_impl(basic_winnt_string&& other, true_type) {
    destroy();
    if (!other.is_small()) {
      m_str = other.m_str;
    } else {
      const size_type new_size{other.size()};
      become_small(new_size);
      traits_type::copy(data(), other.data(), new_size);
    }
    if constexpr (MoveAlloc) {
      get_alloc() = move(other.get_alloc());
    }
    other.become_small();
  }

  template <class Handler, typename... Types>
  void concat_with_optimal_growth(Handler handler,
                                  size_type count,
                                  const Types&... args) {
    concat_impl(
        handler,
        [this](size_type required) { reserve(calc_optimal_growth(required)); },
        count, args...);
  }

  template <class Handler, typename... Types>
  void concat(Handler handler, size_type count, const Types&... args) {
    concat_impl(
        handler, [this](size_type required) { reserve(required); }, count,
        args...);
  }

  template <class Handler, typename... Types>
  void concat_without_transfer_old_data(Handler handler,
                                        size_type count,
                                        const Types&... args) {
    concat_impl(
        handler,
        [this](size_type required) {
          grow_if_needed(required, make_dummy_transfer());
        },
        count, args...);
  }

  template <class Handler, class Reallocator, typename... Types>
  void concat_impl(Handler handler,
                   Reallocator reallocator,
                   size_type count,
                   const Types&... args) {
    size_type old_size{size()};
    reallocator(old_size + count);
    handler(data() + old_size, count, args...);
    native_string_traits_type::increase_size(get_native_str(), count);
  }

  template <class Handler, typename... Types>
  void insert_impl(size_type index,
                   Handler handler,
                   size_type count,
                   const Types&... args) {
    const auto old_size{size()},
        required{static_cast<size_type>(old_size + count)},
        need_to_shift{static_cast<size_type>(old_size - index)};
    value_type* base{data() + index};
    if (required > capacity()) {
      grow(calc_optimal_growth(required),
           make_shifter(make_copy_helper(), {index, count}));
    } else {
      constexpr auto shifter{make_move_helper()};
      shifter(base + count, need_to_shift, base);
    }
    handler(base, count, args...);
    native_string_traits_type::increase_size(get_native_str(), count);
  }

  void erase_unchecked(size_type index, size_type count) {
    auto* first{data() + index};
    const auto need_to_shift{size() - (index + count)};
    traits_type::move(first, first + count, need_to_shift);
    native_string_traits_type::decrease_size(get_native_str(), count);
  }

  template <class TransferPolicy>
  void grow_if_needed(size_type new_capacity, TransferPolicy transfer_handler) {
    if (new_capacity > capacity()) {
      grow(new_capacity, transfer_handler);
    }
  }

  template <class TransferPolicy>
  void grow(size_type new_capacity, TransferPolicy transfer_handler) {
    throw_exception_if_not<length_error>(new_capacity <= max_size(),
                                         L"string too long");
    grow_unchecked(new_capacity, transfer_handler);
  }

  template <class TransferPolicy>
  void grow_unchecked(size_type new_capacity, TransferPolicy transfer_handler) {
    auto& alc{get_alloc()};
    value_type* old_buffer{data()};
    const size_type old_size{size()};
    value_type* new_buffer{new_capacity <= SSO_BUFFER_CH_COUNT
                               ? m_buffer
                               : allocate_buffer(alc, new_capacity)};
    const size_type new_size{
        transfer_handler(new_buffer, old_size, old_buffer)};
    if (!is_small()) {
      deallocate_buffer(alc, old_buffer, capacity());
    }
    native_string_traits_type::set_buffer(get_native_str(), new_buffer);
    native_string_traits_type::set_size(get_native_str(), new_size);
    native_string_traits_type::set_capacity(get_native_str(), new_capacity);
  }

  void destroy() noexcept {
    if (!is_small()) {
      deallocate_buffer(get_alloc(), data(), capacity());
    }
  }

  constexpr size_type calc_growth() noexcept {
    const size_type current_capacity{capacity()}, max_capacity{max_size()};
    if (current_capacity == max_capacity) {
      return npos;
    }
    return (min)(max_capacity, calc_growth_unchecked(current_capacity));
  }

  constexpr size_type calc_optimal_growth(size_type required) noexcept {
    return (max)(calc_growth(), required);
  }

  template <class Buffer>
  static decltype(auto) at_index_verified(Buffer& buf,
                                          size_type size,
                                          size_type idx) {
    throw_out_of_range_if_not(idx < size, "index is out of range");
    return buf[idx];
  }

  template <class Handler>
  static void shift_right_helper(
      Handler handler,
      value_type* dst,
      const value_type* src,
      size_type count,
      pair<size_type, size_type> bounds) noexcept(noexcept(handler(dst,
                                                                   count,
                                                                   src))) {
    handler(dst, bounds.first, src);
    handler(dst + bounds.second, count - bounds.first, src + bounds.first);
  }

  template <class Handler>
  static constexpr auto make_shifter(Handler handler,
                                     pair<size_type, size_type> bounds) {
    return
        [handler, bounds](
            value_type* dst, size_type count,
            const value_type* src) noexcept(noexcept(handler(dst, count, src)))
            -> size_type {
          shift_right_helper(handler, dst, src, count, bounds);
          return count + (bounds.second - bounds.first);
        };
  }

  static size_type transfer_without_shift(value_type* dst,
                                          size_type count,
                                          const value_type* src) noexcept {
    make_copy_helper()(dst, count, src);
    return count;
  }

  static constexpr auto make_dummy_transfer() noexcept {
    return
        []([[maybe_unused]] value_type* dst, size_type count,
           [[maybe_unused]] const value_type* src) noexcept { return count; };
  }

  static constexpr auto make_copy_helper() noexcept {
    return
        [](value_type* dst, size_type count, const value_type* src) noexcept {
          traits_type::copy(dst, src, count);
        };
  }

  template <class InputIt>
  static constexpr auto make_copy_range_helper() noexcept {
    return [](value_type* dst, InputIt first, InputIt last) {
      for (; first != last; first = next(last), ++dst) {
        traits_type::assign(*dst, *first);
      }
    };
  }

  static constexpr auto make_move_helper() noexcept {
    return
        [](value_type* dst, size_type count, const value_type* src) noexcept {
          traits_type::move(dst, src, count);
        };
  }

  static constexpr auto make_fill_helper() noexcept {
    return [](value_type* dst, size_type count, value_type character) noexcept {
      traits_type::assign(dst, count, character);
    };
  }

  static constexpr auto make_ch_comparator() noexcept {
    return [](value_type lhs, value_type rhs) noexcept {
      return traits_type::eq(lhs, rhs);
    };
  }

  static value_type* allocate_buffer(allocator_type& alc, size_type ch_count) {
    return allocator_traits_type::allocate(alc, ch_count);
  }

  static void deallocate_buffer(allocator_type& alc,
                                value_type* buffer,
                                size_type ch_count) noexcept {
    return allocator_traits_type::deallocate(alc, buffer, ch_count);
  }

  static constexpr size_type calc_growth_unchecked(
      size_type old_capacity) noexcept {
    return old_capacity * GROWTH_MULTIPLIER;
  }

  static constexpr int compare_unchecked(const value_type* lhs,
                                         size_type lhs_count,
                                         const value_type* rhs,
                                         size_type rhs_count) noexcept {
    int cmp_result{traits_type::compare(lhs, rhs, (min)(lhs_count, rhs_count))};
    if (cmp_result == 0 && lhs_count != rhs_count) {
      cmp_result = lhs_count < rhs_count ? -1 : 1;
    }
    return cmp_result;
  }

  template <class Ty>
  static void throw_out_of_range_if_not(const Ty& cond) {
    throw_exception_if_not<out_of_range>(cond, L"pos is out of range",
                                         constexpr_message_tag{});
  }

 private:
  compressed_pair<allocator_type, native_string_type> m_str{};
  value_type m_buffer[SsoBufferChCount];
};

template <size_t LhsBufferSize,
          size_t RhsBufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class LhsChAlloc,
          template <typename... CharType>
          class RhsChAlloc>
auto operator+(
    const basic_winnt_string<LhsBufferSize, NativeStrTy, ChTraits, LhsChAlloc>&
        lhs,
    const basic_winnt_string<RhsBufferSize, NativeStrTy, ChTraits, RhsChAlloc>&
        rhs) {
  if constexpr (LhsBufferSize >= RhsBufferSize) {
    basic_winnt_string<LhsBufferSize, NativeStrTy, ChTraits, LhsChAlloc> str{
        lhs};
    str += rhs;
    return str;
  } else {
    basic_winnt_string<RhsBufferSize, NativeStrTy, ChTraits, RhsChAlloc> str{
        rhs};
    str += lhs;
    return str;
  }
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
auto operator+(
    const basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>& lhs,
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        const_pointer null_terminated_str) {
  basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc> str{lhs};
  str += null_terminated_str;
  return str;
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
auto operator+(
    const basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>& lhs,
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        value_type ch) {
  basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc> str{lhs};
  str.push_back(ch);
  return str;
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
auto operator+(
    basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>&& lhs,
    const typename basic_winnt_string<BufferSize,
                                      NativeStrTy,
                                      ChTraits,
                                      ChAlloc>::value_type*
        null_terminated_str) {
  return move(lhs += null_terminated_str);
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
auto operator+(
    basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>&& lhs,
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        value_type ch) {
  basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc> str{move(lhs)};
  str.push_back(ch);
  return str;
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
auto operator+(
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        const_pointer null_terminated_str,
    const basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>& rhs) {
  basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc> str{rhs};
  str += null_terminated_str;
  return str;
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
auto operator+(
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        value_type ch,
    const basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>& rhs) {
  basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc> str{rhs};
  str.push_back(ch);
  return str;
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
auto operator+(
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        const_pointer null_terminated_str,
    basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>&& rhs) {
  return move(rhs.insert(0, null_terminated_str));
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
auto operator+(
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        value_type ch,
    basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>&& rhs) {
  return move(rhs.insert(0, 1, ch));
}

template <size_t LhsBufferSize,
          size_t RhsBufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class LhsChAlloc,
          template <typename... CharType>
          class RhsChAlloc>
auto operator+(
    basic_winnt_string<LhsBufferSize, NativeStrTy, ChTraits, LhsChAlloc>&& lhs,
    const basic_winnt_string<RhsBufferSize, NativeStrTy, ChTraits, RhsChAlloc>&
        rhs) {
  return move(lhs += rhs);
}

template <size_t LhsBufferSize,
          size_t RhsBufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class RhsChTraits,
          template <typename... CharType>
          class LhsChTraits,
          template <typename... CharType>
          class LhsChAlloc,
          template <typename... CharType>
          class RhsChAlloc>
auto operator+(
    const basic_winnt_string<LhsBufferSize,
                             NativeStrTy,
                             LhsChTraits,
                             LhsChAlloc>& lhs,
    basic_winnt_string<RhsBufferSize, NativeStrTy, RhsChTraits, RhsChAlloc>&&
        rhs) {
  return move(rhs += lhs);
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
auto operator+(
    basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>&& lhs,
    basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>&& rhs) {
  using string_type =
      basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>;
  using size_type = typename string_type::size_type;

  assert_with_msg(
      addressof(lhs) != addressof(rhs),
      L"Unable to concatenate the same moved string to itself. See N4849 "
      "[res.on.arguments]/1.3: If a function argument binds to an rvalue "
      "reference parameter, the implementation may assume that this "
      "parameter is a unique reference to this argument");

  const size_type lhs_size{lhs.size()}, rhs_size{rhs.size()},
      lhs_capacity{lhs.capacity()}, rhs_capacity{rhs.capacity()};

  if (lhs_capacity >= rhs_capacity) {
    if (lhs_capacity >= lhs_size + rhs_size) {
      return move(lhs += rhs);
    }
  } else {
    if (rhs_capacity >= lhs_size + rhs_size) {
      return move(rhs += lhs);
    }
  }
  string_type str{};
  str.reserve(lhs_size + rhs_size);
  str += lhs;
  str += rhs;
  return str;
}

template <size_t LhsBufferSize,
          size_t RhsBufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class LhsChAlloc,
          template <typename... CharType>
          class RhsChAlloc>
constexpr bool operator==(
    const basic_winnt_string<LhsBufferSize, NativeStrTy, ChTraits, LhsChAlloc>&
        lhs,
    const basic_winnt_string<RhsBufferSize, NativeStrTy, ChTraits, RhsChAlloc>&
        rhs) noexcept {
  return lhs.compare(rhs) == 0;
}

template <size_t LhsBufferSize,
          size_t RhsBufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class LhsChAlloc,
          template <typename... CharType>
          class RhsChAlloc>
constexpr bool operator!=(
    const basic_winnt_string<LhsBufferSize, NativeStrTy, ChTraits, LhsChAlloc>&
        lhs,
    const basic_winnt_string<RhsBufferSize, NativeStrTy, ChTraits, RhsChAlloc>&
        rhs) noexcept {
  return !(lhs == rhs);
}

template <size_t LhsBufferSize,
          size_t RhsBufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class LhsChAlloc,
          template <typename... CharType>
          class RhsChAlloc>
constexpr bool operator<(
    const basic_winnt_string<LhsBufferSize, NativeStrTy, ChTraits, LhsChAlloc>&
        lhs,
    const basic_winnt_string<RhsBufferSize, NativeStrTy, ChTraits, RhsChAlloc>&
        rhs) noexcept {
  return lhs.compare(rhs) < 0;
}

template <size_t LhsBufferSize,
          size_t RhsBufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class LhsChAlloc,
          template <typename... CharType>
          class RhsChAlloc>
constexpr bool operator>(
    const basic_winnt_string<LhsBufferSize, NativeStrTy, ChTraits, LhsChAlloc>&
        lhs,
    const basic_winnt_string<RhsBufferSize, NativeStrTy, ChTraits, RhsChAlloc>&
        rhs) noexcept {
  return lhs.compare(rhs) > 0;
}

template <size_t LhsBufferSize,
          size_t RhsBufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class LhsChAlloc,
          template <typename... CharType>
          class RhsChAlloc>
constexpr bool operator<=(
    const basic_winnt_string<LhsBufferSize, NativeStrTy, ChTraits, LhsChAlloc>&
        lhs,
    const basic_winnt_string<RhsBufferSize, NativeStrTy, ChTraits, RhsChAlloc>&
        rhs) noexcept {
  return !(lhs > rhs);
}

template <size_t LhsBufferSize,
          size_t RhsBufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class LhsChAlloc,
          template <typename... CharType>
          class RhsChAlloc>
constexpr bool operator>=(
    const basic_winnt_string<LhsBufferSize, NativeStrTy, ChTraits, LhsChAlloc>&
        lhs,
    const basic_winnt_string<RhsBufferSize, NativeStrTy, ChTraits, RhsChAlloc>&
        rhs) noexcept {
  return !(rhs > lhs);
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
constexpr bool operator==(
    const basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>& lhs,
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        const_pointer null_terminated_str) noexcept {
  return lhs.compare(null_terminated_str) == 0;
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
constexpr bool operator==(
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        const_pointer null_terminated_str,
    const basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>&
        rhs) noexcept {
  return rhs.compare(null_terminated_str) == 0;
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
constexpr bool operator!=(
    const basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>& lhs,
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        const_pointer null_terminated_str) noexcept {
  return !(lhs == null_terminated_str);
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
constexpr bool operator!=(
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        const_pointer null_terminated_str,
    const basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>&
        rhs) noexcept {
  return !(null_terminated_str == rhs);
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
constexpr bool operator<(
    const basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>& lhs,
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        const_pointer null_terminated_str) noexcept {
  return lhs.compare(null_terminated_str) < 0;
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
constexpr bool operator<(
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        const_pointer null_terminated_str,
    const basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>&
        rhs) noexcept {
  return lhs.compare(null_terminated_str) < 0;
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
constexpr bool operator>(
    const basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>& lhs,
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        const_pointer null_terminated_str) noexcept {
  return lhs.compare(null_terminated_str) > 0;
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
constexpr bool operator>(
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        const_pointer null_terminated_str,
    const basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>&
        rhs) noexcept {
  return lhs.compare(null_terminated_str) > 0;
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
constexpr bool operator<=(
    const basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>& lhs,
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        const_pointer null_terminated_str) noexcept {
  return !(lhs > null_terminated_str);
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
constexpr bool operator<=(
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        const_pointer null_terminated_str,
    const basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>&
        rhs) noexcept {
  return !(null_terminated_str > rhs);
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
constexpr bool operator>=(
    const basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>& lhs,
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        const_pointer null_terminated_str) noexcept {
  return !(lhs < null_terminated_str);
}

template <size_t BufferSize,
          typename NativeStrTy,
          template <typename... CharType>
          class ChTraits,
          template <typename... CharType>
          class ChAlloc>
constexpr bool operator>=(
    typename basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>::
        const_pointer null_terminated_str,
    const basic_winnt_string<BufferSize, NativeStrTy, ChTraits, ChAlloc>&
        rhs) noexcept {
  return !(null_terminated_str < rhs);
}
}  // namespace ktl::str::details