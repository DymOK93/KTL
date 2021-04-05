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
}  // namespace ktl

// TODO: ktl::reverse_iterator