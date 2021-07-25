#pragma once

#include <functional.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

namespace ktl {
template <class Ty>
void intrusive_ptr_add_ref(Ty*);

template <class Ty>
void intrusive_ptr_release(Ty*) noexcept;

namespace details {
template <class Ty, class = void>
struct is_reference_countable : ktl::false_type {};

template <class Ty>
struct is_reference_countable<
    Ty,
    ktl::void_t<decltype(intrusive_ptr_add_ref(
                    ktl::declval<ktl::add_pointer_t<Ty>>())),
                decltype(intrusive_ptr_release(
                    ktl::declval<ktl::add_pointer_t<Ty>>()))>>
    : ktl::true_type {};

template <class Ty>
inline constexpr bool is_reference_countable_v =
    is_reference_countable<Ty>::value;
}  // namespace details

template <class Ty>
class intrusive_ptr {
 public:
  using element_type = Ty;

  static_assert(
      details::is_reference_countable_v<Ty>,
      "Ty must specialize intrusive_ptr_add_ref() and intrusive_ptr_release()");

 public:
  intrusive_ptr() noexcept = default;

  intrusive_ptr(Ty* ptr, bool add_ref = true) : m_ptr{ptr} {
    if (ptr && add_ref) {
      intrusive_ptr_add_ref(get());
    }
  }

  intrusive_ptr(const intrusive_ptr& other) : m_ptr{other.get()} {
    if (auto* ptr = get(); ptr) {
      intrusive_ptr_add_ref(ptr);
    }
  }

  intrusive_ptr(intrusive_ptr&& other) noexcept : m_ptr{other.detach()} {}

  template <class U, ktl::enable_if_t<ktl::is_convertible_v<U, Ty>, int> = 0>
  intrusive_ptr(const intrusive_ptr<U>& other)
      : m_ptr{static_cast<Ty*>(other.get())} {
    if (auto* ptr = get(); ptr) {
      intrusive_ptr_add_ref(ptr);
    }
  }

  template <class U, ktl::enable_if_t<ktl::is_convertible_v<U, Ty>, int> = 0>
  intrusive_ptr(intrusive_ptr&& other) noexcept
      : m_ptr{static_cast<Ty*>(other.detach())} {}

  ~intrusive_ptr() noexcept { reset(); }

  intrusive_ptr& operator=(const intrusive_ptr& other) {
    if (this != ktl::addressof(other)) {
      intrusive_ptr{other}.swap(*this);
    }
    return *this;
  }

  intrusive_ptr& operator=(intrusive_ptr&& other) noexcept {
    if (this != ktl::addressof(other)) {
      reset(other.detach(), false);
    }
    return *this;
  }

  template <class U>
  intrusive_ptr& operator=(const intrusive_ptr<U>& other) {
    intrusive_ptr{other}.swap(*this);
    return *this;
  }

  template <class U>
  intrusive_ptr& operator=(intrusive_ptr<U>&& other) noexcept {
    if (this != ktl::addressof(other)) {
      reset(static_cast<Ty*>(other.detach(), false));
    }
    return *this;
  }

  intrusive_ptr& operator=(Ty* ptr) {
    reset(ptr);
    return *this;
  }

  void reset(Ty* ptr = nullptr) { intrusive_ptr{ptr}.swap(*this); }
  void reset(Ty* ptr, bool add_ref) { intrusive_ptr{ptr, add_ref}.swap(*this); }

  Ty& operator*() const& noexcept { return *m_ptr; }
  Ty&& operator*() const&& noexcept { return ktl::move(*m_ptr); }
  Ty* operator->() const noexcept { return m_ptr; }
  Ty* get() const noexcept { return m_ptr; }

  Ty* detach() noexcept { return ktl::exchange(m_ptr, nullptr); }

  explicit operator bool() const noexcept { return m_ptr != nullptr; }

  void swap(intrusive_ptr& other) noexcept { ktl::swap(m_ptr, other.m_ptr); }

 private:
  Ty* m_ptr{nullptr};
};

template <class Ty, class U>
bool operator==(const intrusive_ptr<Ty>& lhs,
                const intrusive_ptr<U>& rhs) noexcept {
  return ktl::equal_to<const Ty*>(lhs.get(), rhs.get());
}

template <class Ty, class U>
bool operator!=(const intrusive_ptr<Ty>& lhs,
                const intrusive_ptr<U>& rhs) noexcept {
  return !(lhs == rhs);
}

template <class Ty>
bool operator==(const intrusive_ptr<Ty>& lhs, Ty* rhs) noexcept {
  return ktl::equal_to<const Ty*>(lhs.get(), rhs);
}

template <class Ty>
bool operator!=(const intrusive_ptr<Ty>& lhs, Ty* rhs) noexcept {
  return !(lhs == rhs);
}

template <class Ty>
bool operator==(Ty* lhs, const intrusive_ptr<Ty>& rhs) noexcept {
  return rhs == lhs;
}

template <class Ty>
bool operator!=(Ty* lhs, intrusive_ptr<Ty> const& rhs) noexcept {
  return rhs != lhs;
}

template <class Ty, class U>
bool operator<(const intrusive_ptr<Ty>& lhs,
               const intrusive_ptr<U>& rhs) noexcept {
  using common_ptr_t = ktl::common_type_t<const Ty*, const U*>;
  return ktl::less<common_ptr_t>(static_cast<common_ptr_t>(lhs.get()),
                                 static_cast<common_ptr_t>(rhs.get()));
}

template <class Ty>
void swap(intrusive_ptr<Ty>& lhs, intrusive_ptr<Ty>& rhs) noexcept {
  lhs.swap(rhs);
}

template <class Ty>
Ty* get_pointer(const intrusive_ptr<Ty>& ptr) noexcept {
  return ptr.get();
}
}  // namespace flt