#pragma once

#ifdef KTL_NO_CXX_STANDARD_LIBRARY
#ifndef _INITIALIZER_LIST_
namespace std {
template <class Ty>
class initializer_list {
 public:
  using value_type = Ty;
  using reference = const Ty&;
  using const_reference = const Ty&;
  using size_type = size_t;

  using iterator = const Ty*;
  using const_iterator = const Ty*;

 public:
  constexpr initializer_list() = default;

  constexpr initializer_list(const Ty* first, const Ty* last) noexcept
      : m_first{first}, m_last{last} {}

  [[nodiscard]] constexpr const Ty* begin() const noexcept { return m_first; }

  [[nodiscard]] constexpr const Ty* end() const noexcept { return m_last; }

  [[nodiscard]] constexpr size_t size() const noexcept {
    return static_cast<size_t>(m_last - m_first);
  }

 private:
  const Ty* m_first{nullptr};
  const Ty* m_last{nullptr};
};

template <class Ty>
[[nodiscard]] constexpr const Ty* begin(
    initializer_list<Ty> init_list) noexcept {
  return init_list.begin();
}

template <class Ty>
[[nodiscard]] constexpr const Ty* end(initializer_list<Ty> init_list) noexcept {
  return init_list.end();
}
}  // namespace std
#endif
#endif

namespace ktl {
    using std::initializer_list;
}