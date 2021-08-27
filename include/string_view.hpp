#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <algorithm>
#include <string_view>
#else
#include <container_helpers.h>
#include <ktlexcept.hpp>
#include <algorithm.hpp>
#include <assert.hpp>
#include <iterator.hpp>
#include <string_algorithm_impl.hpp>
#include <string_fwd.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

namespace ktl {
template <typename CharT, class Traits>
class basic_winnt_string_view {
 public:
  using native_string_traits_type =
      str::details::native_string_traits_selector<CharT>;
  using native_string_type = typename native_string_traits_type::string_type;

  using value_type = typename native_string_traits_type::value_type;
  using size_type = typename native_string_traits_type::size_type;

  using traits_type = Traits;

  using difference_type = ptrdiff_t;

  using const_reference = const value_type&;
  using reference = const_reference;
  using const_pointer = const value_type*;
  using pointer = const_pointer;

  using const_iterator = const_pointer;
  using iterator = const_iterator;

 public:
  static constexpr auto npos{static_cast<size_type>(-1)};

 public:
  constexpr basic_winnt_string_view() noexcept = default;

  constexpr basic_winnt_string_view(
      const basic_winnt_string_view& other) noexcept = default;

  constexpr basic_winnt_string_view& operator=(
      const basic_winnt_string_view& other) noexcept = default;

  template <size_t N>
  constexpr basic_winnt_string_view(
      const value_type (&null_terminated_str)[N + 1]) noexcept
      : m_str{make_native_str(null_terminated_str, static_cast<size_type>(N))} {
  }

  constexpr basic_winnt_string_view(
      const value_type* null_terminated_str) noexcept
      : m_str{make_native_str(
            null_terminated_str,
            static_cast<size_type>(traits_type::length(null_terminated_str)))} {
  }

  constexpr basic_winnt_string_view(const value_type* str,
                                    size_type length) noexcept
      : m_str{make_native_str(str, length)} {}

  constexpr basic_winnt_string_view(native_string_type native_str)
      : m_str{native_str} {}

 public:
  constexpr const_iterator begin() const noexcept { return cbegin(); }
  constexpr const_iterator cbegin() const noexcept { return data(); }
  constexpr const_iterator end() const noexcept { return cend(); }
  constexpr const_iterator cend() const noexcept { return data() + length(); }

 public:
  value_type& at(size_type idx) const {
    return cont::details::at_index_verified(data(), idx, size());
  }

  constexpr const value_type& operator[](size_type idx) const noexcept {
    assert_with_msg(idx < size(), L"index is out of range");
    return data()[idx];
  }

  constexpr const value_type& front() const noexcept {
    assert_with_msg(!empty(), L"front() called on empty string_view");
    return data()[0];
  }

  constexpr const value_type& back() const noexcept {
    assert_with_msg(!empty(), L"front() called at empty string_view");
    return data()[size() - 1];
  }

  constexpr const value_type* data() const noexcept {
    return native_string_traits_type::get_buffer(m_str);
  }

  constexpr bool empty() const noexcept { return size() == 0; }

  constexpr size_type size() const noexcept {
    return native_string_traits_type::get_size(m_str);
  }

  constexpr size_type length() const noexcept { return size(); }

  constexpr void remove_prefix(size_type count) noexcept {
    assert_with_msg(count <= size(), L"prefix is too long");
    native_string_traits_type::set_buffer(
        m_str, const_cast<value_type*>(data() + count));
    native_string_traits_type::decrease_size(m_str, +count);
  }

  constexpr void remove_suffix(size_type count) noexcept {
    assert_with_msg(count <= size(), L"suffix is too long");
    native_string_traits_type::decrease_size(m_str, count);
  }

  constexpr void swap(basic_winnt_string_view& other) noexcept {
    using ::swap;
    swap(m_str, other.m_str);
  }

  constexpr basic_winnt_string_view substr(
      size_type pos,
      size_type count = npos) const noexcept {
    cont::details::throw_if_index_greater_than_size(pos, size());
    return substr_unchecked(pos, count);
  }

  constexpr int compare(basic_winnt_string_view other) const noexcept {
    return compare_unchecked(data(), size(), other.data(), other.size());
  }

  constexpr int compare(size_type pos,
                        size_type count,
                        basic_winnt_string_view other) const {
    return substr(pos, count).compare(other);
  }

  constexpr int compare(size_type pos,
                        size_type count,
                        basic_winnt_string_view other,
                        size_type other_pos,
                        size_type other_count) const {
    return substr(pos, count).compare(other.substr(other_pos, other_count));
  }
  constexpr int compare(const value_type* other) const noexcept {
    return compare_unchecked(data(), size(), other, traits_type::length(other));
  }

  constexpr int compare(size_type pos,
                        size_type count,
                        const value_type* other) const {
    return substr(pos, count).compare(other);
  }

  constexpr int compare(size_type pos,
                        size_type count,
                        const value_type* other,
                        size_type other_pos,
                        size_type other_count) const {
    return substr(pos, count)
        .compare(basic_winnt_string_view(other + other_pos, other_count));
  }

  constexpr bool starts_with(basic_winnt_string_view other) const noexcept {
    const size_type other_size{other.size()};
    if (other_size > size()) {
      return false;
    }
    return compare_unchecked(data(), other_size, other.data(), other_size) == 0;
  }

  constexpr bool starts_with(value_type ch) const noexcept {
    return !empty() && traits_type::eq(front(), ch);
  }

  constexpr bool starts_with(const value_type* null_terminated_str) {
    return starts_with(basic_winnt_string_view{null_terminated_str});
  }

  constexpr bool ends_with(basic_winnt_string_view other) const noexcept {
    const size_type current_size{size()}, other_size{other.size()};
    if (other_size > current_size) {
      return false;
    }
    return substr_unchecked(current_size - other_size, other_size)
        .compare(other);
  };

  constexpr bool ends_with(value_type ch) const noexcept {
    return !empty() && traits_type::eq(back(), ch);
  }

  constexpr bool ends_with(
      const value_type* null_terminated_str) const noexcept {
    return ends_with(basic_winnt_string_view{null_terminated_str});
  }

  constexpr bool contains(basic_winnt_string_view other) const noexcept {
    return find(other) != npos;
  }

  constexpr bool contains(value_type ch) const noexcept {
    return find(ch) != npos;
  }

  constexpr bool contains(
      const value_type* null_terminated_str) const noexcept {
    return find(null_terminated_str) != npos;
  }

  constexpr size_type find(basic_winnt_string_view other,
                           size_type my_pos = 0) const noexcept {
    return str::details::find_substr<traits_type>(
        data(), my_pos, size(), other.data(), other.size(), npos);
  }

  constexpr size_type find(value_type ch, size_type my_pos = 0) const noexcept {
    return str::details::find_ch<traits_type>(data(), ch, size(), my_pos, npos);
  }

  constexpr size_type find(const value_type* null_terminated_str,
                           size_type my_pos = 0) const noexcept {
    return find(basic_winnt_string_view{null_terminated_str}, my_pos);
  }

  constexpr size_type find(native_string_type native_str) const noexcept {
    return find(basic_winnt_string_view{native_str});
  }

  constexpr size_type find(const value_type* str,
                           size_t my_pos,
                           size_type other_count) const noexcept {
    return find(basic_winnt_string_view{str, other_count}, my_pos);
  }

  const native_string_type* raw_str() const noexcept {
    return addressof(m_str);
  }

  constexpr size_type find_first_of(basic_winnt_string_view other,
                                    size_type pos = 0) const noexcept {
    return find(other, pos);
  }

  constexpr size_type find_first_of(value_type ch,
                                    size_type pos = 0) const noexcept {
    return find(ch, pos);
  }

  constexpr size_type find_first_of(const value_type* null_terminated_str,
                                    size_type my_pos,
                                    size_type other_count) const noexcept {
    return find(null_terminated_str, my_pos, other_count);
  }

  constexpr size_type find_first_of(const value_type* null_terminated_str,
                                    size_type my_pos = 0) const {
    return find(null_terminated_str, my_pos);
  }

  size_type copy(value_type* dst, size_type count, size_type pos = 0) const {
    const size_type current_size{size()};
    cont::details::throw_if_index_greater_than_size(pos, current_size);
    const auto copied{(min)(static_cast<size_type>(current_size - pos), count)};
    traits_type::copy(dst, data() + pos,
                      copied);  // Пользователь отвечает за то, что диапазоны не
                                // должны пересекаться
    return copied;
  }

 private:
  constexpr native_string_type make_native_str(
      const value_type* str,
      size_type length) noexcept {  // length in characters
    native_string_type native_str{};
    native_string_traits_type::set_buffer(native_str,
                                          const_cast<value_type*>(str));
    native_string_traits_type::set_size(native_str, length);
    native_string_traits_type::set_capacity(native_str, length);
    return native_str;
  }

  constexpr basic_winnt_string_view substr_unchecked(
      size_type pos,
      size_type count) const noexcept {
    const auto rest{static_cast<size_type>(size() - pos)};
    return {data() + pos, (min)(rest, count)};
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

  static constexpr auto make_ch_comparator() noexcept {
    return [](value_type lhs, value_type rhs) noexcept {
      return traits_type::eq(lhs, rhs);
    };
  }

 private:
  native_string_type m_str{};
};

template <class CharT>
basic_winnt_string_view(const CharT*) -> basic_winnt_string_view<CharT>;

template <class NativeStrTy,
          enable_if_t<cont::details::has_value_type_v<
                          str::details::native_string_traits<NativeStrTy> >,
                      int> = 0>
basic_winnt_string_view(NativeStrTy) -> basic_winnt_string_view<
    typename str::details::native_string_traits<NativeStrTy>::value_type>;

template <class CharT, class ChTraits>
constexpr bool operator==(
    const basic_winnt_string_view<CharT, ChTraits> lhs,
    const basic_winnt_string_view<CharT, ChTraits> rhs) noexcept {
  return lhs.compare(rhs) == 0;
}

template <
    class CharT,
    class ChTraits,
    class Ty,
    enable_if_t<
        is_convertible_v<const Ty&, basic_winnt_string_view<CharT, ChTraits> >,
        int> = 0>
constexpr bool operator==(const basic_winnt_string_view<CharT, ChTraits> lhs,
                          const Ty& rhs) noexcept {
  return lhs == basic_winnt_string_view<CharT, ChTraits>{rhs};
}

template <
    class Ty,
    class CharT,
    class ChTraits,
    enable_if_t<
        is_convertible_v<const Ty&, basic_winnt_string_view<CharT, ChTraits> >,
        int> = 0>
constexpr bool operator==(
    const Ty& lhs,
    const basic_winnt_string_view<CharT, ChTraits> rhs) noexcept {
  return basic_winnt_string_view<CharT, ChTraits>{lhs} == rhs;
}

template <class CharT, class ChTraits>
constexpr bool operator!=(
    basic_winnt_string_view<CharT, ChTraits> lhs,
    basic_winnt_string_view<CharT, ChTraits> rhs) noexcept {
  return lhs.compare(rhs) != 0;
}

template <
    class CharT,
    class ChTraits,
    class Ty,
    enable_if_t<
        is_convertible_v<const Ty&, basic_winnt_string_view<CharT, ChTraits> >,
        int> = 0>
constexpr bool operator!=(const basic_winnt_string_view<CharT, ChTraits> lhs,
                          const Ty& rhs) noexcept {
  return lhs != basic_winnt_string_view<CharT, ChTraits>{rhs};
}

template <
    class Ty,
    class CharT,
    class ChTraits,
    enable_if_t<
        is_convertible_v<const Ty&, basic_winnt_string_view<CharT, ChTraits> >,
        int> = 0>
constexpr bool operator!=(
    const Ty& lhs,
    const basic_winnt_string_view<CharT, ChTraits> rhs) noexcept {
  return basic_winnt_string_view<CharT, ChTraits>{lhs} != rhs;
}

template <class CharT, class ChTraits>
constexpr bool operator<(
    basic_winnt_string_view<CharT, ChTraits> lhs,
    basic_winnt_string_view<CharT, ChTraits> rhs) noexcept {
  return lhs.compare(rhs) < 0;
}

template <
    class CharT,
    class ChTraits,
    class Ty,
    enable_if_t<
        is_convertible_v<const Ty&, basic_winnt_string_view<CharT, ChTraits> >,
        int> = 0>
constexpr bool operator<(const basic_winnt_string_view<CharT, ChTraits> lhs,
                         const Ty& rhs) noexcept {
  return lhs < basic_winnt_string_view<CharT, ChTraits>{rhs};
}

template <
    class Ty,
    class CharT,
    class ChTraits,
    enable_if_t<
        is_convertible_v<const Ty&, basic_winnt_string_view<CharT, ChTraits> >,
        int> = 0>
constexpr bool operator<(
    const Ty& lhs,
    const basic_winnt_string_view<CharT, ChTraits> rhs) noexcept {
  return basic_winnt_string_view<CharT, ChTraits>{lhs} < rhs;
}

template <class CharT, class ChTraits>
constexpr bool operator<=(
    basic_winnt_string_view<CharT, ChTraits> lhs,
    basic_winnt_string_view<CharT, ChTraits> rhs) noexcept {
  return !(lhs > rhs);
}

template <
    class CharT,
    class ChTraits,
    class Ty,
    enable_if_t<
        is_convertible_v<const Ty&, basic_winnt_string_view<CharT, ChTraits> >,
        int> = 0>
constexpr bool operator<=(const basic_winnt_string_view<CharT, ChTraits> lhs,
                          const Ty& rhs) noexcept {
  return lhs <= basic_winnt_string_view<CharT, ChTraits>{rhs};
}

template <
    class Ty,
    class CharT,
    class ChTraits,
    enable_if_t<
        is_convertible_v<const Ty&, basic_winnt_string_view<CharT, ChTraits> >,
        int> = 0>
constexpr bool operator<=(
    const Ty& lhs,
    const basic_winnt_string_view<CharT, ChTraits> rhs) noexcept {
  return basic_winnt_string_view<CharT, ChTraits>{lhs} <= rhs;
}

template <class CharT, class ChTraits>
constexpr bool operator>(
    basic_winnt_string_view<CharT, ChTraits> lhs,
    basic_winnt_string_view<CharT, ChTraits> rhs) noexcept {
  return lhs.compare(rhs) > 0;
}

template <
    class CharT,
    class ChTraits,
    class Ty,
    enable_if_t<
        is_convertible_v<const Ty&, basic_winnt_string_view<CharT, ChTraits> >,
        int> = 0>
constexpr bool operator>(const basic_winnt_string_view<CharT, ChTraits> lhs,
                         const Ty& rhs) noexcept {
  return lhs > basic_winnt_string_view<CharT, ChTraits>{rhs};
}

template <
    class Ty,
    class CharT,
    class ChTraits,
    enable_if_t<
        is_convertible_v<const Ty&, basic_winnt_string_view<CharT, ChTraits> >,
        int> = 0>
constexpr bool operator>(
    const Ty& lhs,
    const basic_winnt_string_view<CharT, ChTraits> rhs) noexcept {
  return basic_winnt_string_view<CharT, ChTraits>{lhs} > rhs;
}

template <class CharT, class ChTraits>
constexpr bool operator>=(
    basic_winnt_string_view<CharT, ChTraits> lhs,
    basic_winnt_string_view<CharT, ChTraits> rhs) noexcept {
  return !(lhs < rhs);
}

template <
    class CharT,
    class ChTraits,
    class Ty,
    enable_if_t<
        is_convertible_v<const Ty&, basic_winnt_string_view<CharT, ChTraits> >,
        int> = 0>
constexpr bool operator>=(const basic_winnt_string_view<CharT, ChTraits> lhs,
                          const Ty& rhs) noexcept {
  return lhs >= basic_winnt_string_view<CharT, ChTraits>{rhs};
}

template <
    class Ty,
    class CharT,
    class ChTraits,
    enable_if_t<
        is_convertible_v<const Ty&, basic_winnt_string_view<CharT, ChTraits> >,
        int> = 0>
constexpr bool operator>=(
    const Ty& lhs,
    const basic_winnt_string_view<CharT, ChTraits> rhs) noexcept {
  return basic_winnt_string_view<CharT, ChTraits>{lhs} >= rhs;
}

inline namespace literals {
inline namespace string_literals {
constexpr ansi_string_view operator""_asv(const char* str,
                                          size_t length) noexcept {
  return {str, static_cast<ansi_string_view::size_type>(length)};
}

constexpr unicode_string_view operator""_usv(const wchar_t* str,
                                             size_t length) noexcept {
  return {str, static_cast<unicode_string_view::size_type>(length)};
}
}  // namespace string_literals
}  // namespace literals
}  // namespace ktl
#endif