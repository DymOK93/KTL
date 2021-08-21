#pragma once

// coroutine standard header (core)

// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#pragma once
#ifdef _RESUMABLE_FUNCTIONS_SUPPORTED
#pragma message("The contents of <coroutine> are not available with /await")
#pragma message( \
    "Remove /await for standard coroutines or use <experimental/coroutine> for legacy /await support")
#else
#ifndef KTL_COROUTINES
#pragma error("The corutines are supported only with C++20 or later")
#else

#include <hash.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

namespace ktl {
namespace coro::details {
template <class RetType, class = void>
struct coroutine_traits {};

template <class RetType>
struct coroutine_traits<RetType, ktl::void_t<typename RetType::promise_type> > {
  using promise_type = typename RetType::promise_type;
};
}  // namespace coro::details

template <class RetType, class...>
struct coroutine_traits : coro::details::coroutine_traits<RetType> {};

// STRUCT TEMPLATE coroutine_handle
template <class = void>
struct coroutine_handle;

template <>
struct coroutine_handle<void> {
  constexpr coroutine_handle() noexcept = default;
  constexpr coroutine_handle(nullptr_t) noexcept {}

  coroutine_handle& operator=(nullptr_t) noexcept {
    m_ptr = nullptr;
    return *this;
  }

  [[nodiscard]] constexpr void* address() const noexcept { return m_ptr; }

  [[nodiscard]] static constexpr coroutine_handle from_address(
      void* const ptr) noexcept {  // strengthened
    coroutine_handle _Result;
    _Result.m_ptr = ptr;
    return _Result;
  }

  constexpr explicit operator bool() const noexcept { return m_ptr != nullptr; }

  [[nodiscard]] bool done() const noexcept {  // strengthened
    return __builtin_coro_done(m_ptr);
  }

  void operator()() const { __builtin_coro_resume(m_ptr); }

  void resume() const { __builtin_coro_resume(m_ptr); }

  void destroy() const noexcept {  // strengthened
    __builtin_coro_destroy(m_ptr);
  }

 private:
  void* m_ptr{nullptr};
};

template <class _Promise>
struct coroutine_handle {
  constexpr coroutine_handle() noexcept = default;
  constexpr coroutine_handle(nullptr_t) noexcept {}

  [[nodiscard]] static coroutine_handle from_promise(
      _Promise& from) noexcept {  // strengthened
    const auto from_ptr = const_cast<void*>(
        static_cast<const volatile void*>(ktl::addressof(from)));
    const auto frame_ptr = __builtin_coro_promise(from_ptr, 0, true);
    coroutine_handle result;
    result.m_ptr = frame_ptr;
    return result;
  }

  coroutine_handle& operator=(nullptr_t) noexcept {
    m_ptr = nullptr;
    return *this;
  }

  [[nodiscard]] constexpr void* address() const noexcept { return m_ptr; }

  [[nodiscard]] static constexpr coroutine_handle from_address(
      void* const ptr) noexcept {
    coroutine_handle result;
    result.m_ptr = ptr;
    return result;
  }

  constexpr operator coroutine_handle<>() const noexcept {
    return coroutine_handle<>::from_address(m_ptr);
  }

  constexpr explicit operator bool() const noexcept { return m_ptr != nullptr; }

  [[nodiscard]] bool done() const noexcept {  // strengthened
    return __builtin_coro_done(m_ptr);
  }

  void operator()() const { __builtin_coro_resume(m_ptr); }

  void resume() const { __builtin_coro_resume(m_ptr); }

  void destroy() const noexcept {  // strengthened
    __builtin_coro_destroy(m_ptr);
  }

  [[nodiscard]] _Promise& promise() const noexcept {  // strengthened
    return *reinterpret_cast<_Promise*>(
        __builtin_coro_promise(m_ptr, 0, false));
  }

 private:
  void* m_ptr = nullptr;
};

[[nodiscard]] constexpr bool operator==(const coroutine_handle<> lhs,
                                        const coroutine_handle<> rhs) noexcept {
  return lhs.address() == rhs.address();
}

[[nodiscard]] constexpr bool operator!=(const coroutine_handle<> lhs,
                                        const coroutine_handle<> rhs) noexcept {
  return !(lhs == rhs);
}

template <class Promise>
struct hash<coroutine_handle<Promise> > {
  [[nodiscard]] size_t operator()(
      const coroutine_handle<Promise>& coro) const noexcept {
    return hash{}(coro.address());
  }
};

struct noop_coroutine_promise {};

template <>
struct coroutine_handle<noop_coroutine_promise> {
  friend coroutine_handle noop_coroutine() noexcept;

  constexpr operator coroutine_handle<>() const noexcept {
    return coroutine_handle<>::from_address(m_ptr);
  }

  constexpr explicit operator bool() const noexcept { return true; }
  [[nodiscard]] constexpr bool done() const noexcept { return false; }

  constexpr void operator()() const noexcept {}
  constexpr void resume() const noexcept {}
  constexpr void destroy() const noexcept {}

  [[nodiscard]] noop_coroutine_promise& promise() const noexcept {
    return *reinterpret_cast<noop_coroutine_promise*>(
        __builtin_coro_promise(m_ptr, 0, false));
  }

  [[nodiscard]] constexpr void* address() const noexcept { return m_ptr; }

 private:
  coroutine_handle() noexcept = default;

 private:
  void* m_ptr{__builtin_coro_noop()};
};

using noop_coroutine_handle = coroutine_handle<noop_coroutine_promise>;

[[nodiscard]] inline noop_coroutine_handle noop_coroutine() noexcept {
  // Returns a handle to a coroutine that has no observable effects when resumed
  // or destroyed.
  return noop_coroutine_handle{};
}

struct suspend_never {
  [[nodiscard]] constexpr bool await_ready() const noexcept { return true; }

  constexpr void await_suspend(coroutine_handle<>) const noexcept {}
  constexpr void await_resume() const noexcept {}
};

struct suspend_always {
  [[nodiscard]] constexpr bool await_ready() const noexcept { return false; }

  constexpr void await_suspend(coroutine_handle<>) const noexcept {}
  constexpr void await_resume() const noexcept {}
};
}  // namespace ktl

#ifndef __cpp_lib_coroutine
namespace std {
using ktl::coroutine_handle;
using ktl::coroutine_traits;
}  // namespace std
#endif

#endif  // __cpp_lib_coroutine
#endif  // _RESUMABLE_FUNCTIONS_SUPPORTED
