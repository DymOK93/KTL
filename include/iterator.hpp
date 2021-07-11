#pragma once
#include <iterator_impl.hpp>

namespace ktl {
template <class Container>
constexpr decltype(auto) begin(Container& cont) noexcept(
    noexcept(cont.begin())) {
  return cont.begin();
}

template <class Container>
constexpr decltype(auto) cbegin(const Container& cont) noexcept(
    noexcept(cont.begin())) {
  return cont.begin();
}

template <class Ty, size_t N>
constexpr Ty* begin(Ty (&array)[N]) noexcept {
  return array;
}

template <class Ty, size_t N>
constexpr const Ty* begin(const Ty (&array)[N]) noexcept {
  return array;
}

template <class Container>
constexpr decltype(auto) end(Container& cont) noexcept(noexcept(cont.end())) {
  return cont.end();
}

template <class Container>
constexpr decltype(auto) cend(const Container& cont) noexcept(
    noexcept(cont.end())) {
  return cont.end();
}

template <class Ty, size_t N>
constexpr Ty* end(Ty (&array)[N]) noexcept {
  return array + N;
}

template <class Ty, size_t N>
constexpr const Ty* end(const Ty (&array)[N]) noexcept {
  return array + N;
}

template <class It>
class move_iterator {
 private:
  using original_reference = typename iterator_traits<It>::reference;

 public:
  using iterator_type = It;
  using value_type = typename iterator_traits<It>::value_type;
  using difference_type = typename iterator_traits<It>::difference_type;
  using pointer = It;
  using reference = conditional_t<
      is_lvalue_reference_v<original_reference>,
      add_rvalue_reference_t<remove_reference_t<original_reference>>,
      original_reference>;
  using iterator_category = typename iterator_traits<It>::iterator_category;

 public:
  constexpr move_iterator() noexcept(
      is_nothrow_default_constructible_v<iterator_type>) = default;

  constexpr explicit move_iterator(iterator_type it) noexcept(
      is_nothrow_move_constructible_v<iterator_type>)
      : m_current{move(it)} {}

  template <class OtherIt>
  constexpr move_iterator(const move_iterator<OtherIt>& other) noexcept(
      is_nothrow_constructible_v<iterator_type, const OtherIt&>)
      : m_current{other.base()} {}

  template <class OtherIt>
  constexpr move_iterator&
  operator=(const move_iterator<OtherIt>& other) noexcept(
      is_nothrow_assignable_v<iterator_type, const OtherIt&>) {
    m_current = other.base();
    return *this;
  }

  constexpr const iterator_type& base() const& noexcept { return m_current; }
  constexpr iterator_type base() && noexcept { return move(m_current); }

  constexpr reference operator*() const {
    return static_cast<reference>(*m_current);
  }

  constexpr pointer operator->() const { return m_current; }

  constexpr reference operator[](difference_type idx) { move(base()[idx]); }

  move_iterator& operator++() {
    m_current = next(m_current);
    return *this;
  }

  move_iterator operator++(int) {
    auto old_it{*this};
    ++*this;
    return old_it;
  }

  move_iterator& operator+=(difference_type offset) {
    advance(m_current, offset);
    return *this;
  }

  move_iterator operator+(difference_type offset) const {
    auto it_copy{*this};
    it_copy += offset;
    return it_copy;
  }

  move_iterator& operator--() {
    m_current = prev(m_current);
    return *this;
  }

  move_iterator operator--(int) {
    auto old_it{*this};
    --*this;
    return old_it;
  }

  move_iterator& operator-=(difference_type offset) {
    advance(m_current, -offset);
    return *this;
  }

  move_iterator operator-(difference_type offset) const {
    auto it_copy{*this};
    it_copy -= offset;
    return it_copy;
  }

 private:
  iterator_type m_current{};
};

template <class It>
constexpr move_iterator<It> operator+(
    typename move_iterator<It>::difference_type offset,
    const move_iterator<It>& it) {
  return it + offset;
}

template <class LhsIt, class RhsIt>
constexpr decltype(auto) operator-(const move_iterator<LhsIt>& lhs,
                                   const move_iterator<RhsIt>& rhs) {
  return lhs.base() - rhs.base();
}

template <class It>
constexpr move_iterator<It> make_move_iterator(It it) {
  return move_iterator<It>{move(it)};
}

template <class LhsIt, class RhsIt>
constexpr auto operator==(const move_iterator<LhsIt>& lhs,
                          const move_iterator<RhsIt>& rhs)
    -> decltype(is_convertible_v<bool, decltype(lhs.base() == rhs.base())>) {
  return static_cast<bool>(lhs.base() == rhs.base());
}

template <class LhsIt, class RhsIt>
constexpr bool operator!=(const move_iterator<LhsIt>& lhs,
                          const move_iterator<RhsIt>& rhs) {
  return !(lhs == rhs);
}

template <class LhsIt, class RhsIt>
constexpr auto operator<(const move_iterator<LhsIt>& lhs,
                         const move_iterator<RhsIt>& rhs)
    -> decltype(is_convertible_v<bool, decltype(lhs.base() < rhs.base())>) {
  return static_cast<bool>(lhs.base() < rhs.base());
}

template <class LhsIt, class RhsIt>
constexpr auto operator<=(const move_iterator<LhsIt>& lhs,
                          const move_iterator<RhsIt>& rhs)
    -> decltype(is_convertible_v<bool, decltype(lhs.base() <= rhs.base())>) {
  return static_cast<bool>(lhs.base() <= rhs.base());
}

template <class LhsIt, class RhsIt>
constexpr auto operator>(const move_iterator<LhsIt>& lhs,
                         const move_iterator<RhsIt>& rhs)
    -> decltype(is_convertible_v<bool, decltype(lhs.base() > rhs.base())>) {
  return static_cast<bool>(lhs.base() > rhs.base());
}

template <class LhsIt, class RhsIt>
constexpr auto operator>=(const move_iterator<LhsIt>& lhs,
                          const move_iterator<RhsIt>& rhs)
    -> decltype(is_convertible_v<bool, decltype(lhs.base() >= rhs.base())>) {
  return static_cast<bool>(lhs.base() >= rhs.base());
}

template <class Container>
class back_insert_iterator {
 public:
  using iterator_category = output_iterator_tag;
  using value_type = void;
  using difference_type = ptrdiff_t;
  using pointer = void;
  using reference = void;
  using container_type = Container;

 private:
  using containter_value_type = typename Container::value_type;

 public:
  explicit constexpr back_insert_iterator(Container& cont) noexcept
      : container{addressof(cont)} {}

  constexpr back_insert_iterator& operator=(
      const containter_value_type& value) {
    container->push_back(value);
    return *this;
  }

  constexpr back_insert_iterator& operator=(containter_value_type&& value) {
    container->push_back(move(value));
    return *this;
  }

  constexpr back_insert_iterator& operator*() noexcept { return *this; }
  constexpr back_insert_iterator& operator++() noexcept { return *this; }
  constexpr back_insert_iterator operator++(int) noexcept { return *this; }

 protected:
  container_type* container;
};
}  // namespace ktl

// TODO: ktl::reverse_iterator