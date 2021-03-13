#pragma once
#include <basic_types.h>
#include <crt_attributes.h>
#include <intrinsic.hpp>
#include <limits.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

#include <ntddk.h>

namespace ktl {
namespace th {
inline bool interlocked_exchange(bool& target, bool new_value) {
  return InterlockedExchange8(
      reinterpret_cast<volatile char*>(addressof(target)),
      static_cast<char>(new_value));
}

inline void interlocked_swap(bool& lhs, bool& rhs) {
  bool old_lhs = lhs;
  interlocked_exchange(lhs, rhs);
  interlocked_exchange(rhs, old_lhs);
}

template <class Ty>
Ty* interlocked_exchange_pointer(Ty* const* ptr_place, Ty* new_ptr) noexcept {
  return static_cast<Ty*>(InterlockedExchangePointer(
      reinterpret_cast<volatile PVOID*>(const_cast<Ty**>(ptr_place)), new_ptr));
}

template <class Ty>
Ty* interlocked_compare_exchange_pointer(Ty* const* ptr_place,
                                         Ty* new_ptr,
                                         Ty* expected) noexcept {
  return static_cast<Ty*>(InterlockedCompareExchangePointer(
      reinterpret_cast<volatile PVOID*>(const_cast<Ty**>(ptr_place)), new_ptr,
      expected));
}

template <class Ty>
void interlocked_swap_pointer(Ty* const lhs, Ty* const rhs) noexcept {
  Ty* old_lhs = lhs;
  interlocked_exchange(lhs, rhs);
  interlocked_exchange(rhs, old_lhs);
}
}  // namespace th

template <typename IntegralTy, typename Ty>
static volatile IntegralTy* atomic_address_as(Ty& value) noexcept {
  static_assert(is_integral_v<IntegralTy>, "value must be integral");
  return &reinterpret_cast<volatile IntegralTy&>(value);
}

template <typename IntegralTy, typename Ty>
static const volatile IntegralTy* atomic_address_as(const Ty& value) noexcept {
  static_assert(is_integral_v<IntegralTy>, "value must be integral");
  return &reinterpret_cast<const volatile IntegralTy&>(value);
}

namespace th::details {
inline void make_compiler_barrier() noexcept {
  _ReadWriteBarrier();
}

class spin_lock {
 public:
  using lock_type = long;

 public:
  spin_lock() noexcept = default;

  void lock() noexcept {
    while (InterlockedCompareExchange(atomic_address_as<long>(m_lock), 1, 0))
      ;
  }
  void unlock() noexcept {
    InterlockedExchange(atomic_address_as<long>(m_lock), 0);
  }

 private:
  lock_type m_lock{0};
};

template <class Lock>
class auto_lock {
 public:
  auto_lock(Lock& lock) noexcept : m_lock{lock} { m_lock.lock(); }
  ~auto_lock() { m_lock.unlock(); }

 private:
  Lock& m_lock;
};

}  // namespace th::details

// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

enum class memory_order : uint8_t {
  relaxed = 0,
  consume = 1,
  acquire = 2,
  release = 4,
  acq_rel = 6,  // acquire | release
  seq_cst = 14  // acq_rel | 8
};

inline constexpr memory_order memory_order_relaxed = memory_order::relaxed;
inline constexpr memory_order memory_order_consume = memory_order::consume;
inline constexpr memory_order memory_order_acquire = memory_order::acquire;
inline constexpr memory_order memory_order_release = memory_order::release;
inline constexpr memory_order memory_order_acq_rel = memory_order::acq_rel;
inline constexpr memory_order memory_order_seq_cst = memory_order::seq_cst;

namespace th::details {
template <class Ty>
Ty kill_dependency(Ty arg) noexcept {  // "magic" template that kills
                                       // dependency ordering when called
  return arg;
}

template <memory_order order>
void check_store_memory_order() noexcept {}

template <>
void check_store_memory_order<memory_order::consume>() noexcept;
template <>
void check_store_memory_order<memory_order::acquire>() noexcept;
template <>
void check_store_memory_order<memory_order::acq_rel>() noexcept;

template <memory_order order>
void check_load_memory_order() noexcept {}

template <>
void check_load_memory_order<memory_order::release>() noexcept;
template <>
void check_store_memory_order<memory_order::acq_rel>() noexcept;

template <memory_order on_success, memory_order on_failure>
[[nodiscard]] constexpr memory_order combine_cas_memory_orders() noexcept {
  // Finds upper bound of a compare/exchange memory order
  // pair, according to the following partial order:
  //     seq_cst
  //        |
  //     acq_rel
  //     /     \
    // acquire  release
  //    |       |
  // consume    |
  //     \     /
  //     relaxed
  static constexpr memory_order COMBINED[6][6] = {
      {memory_order_relaxed, memory_order_consume, memory_order_acquire,
       memory_order_release, memory_order_acq_rel, memory_order_seq_cst},
      {memory_order_consume, memory_order_consume, memory_order_acquire,
       memory_order_acq_rel, memory_order_acq_rel, memory_order_seq_cst},
      {memory_order_acquire, memory_order_acquire, memory_order_acquire,
       memory_order_acq_rel, memory_order_acq_rel, memory_order_seq_cst},
      {memory_order_release, memory_order_acq_rel, memory_order_acq_rel,
       memory_order_release, memory_order_acq_rel, memory_order_seq_cst},
      {memory_order_acq_rel, memory_order_acq_rel, memory_order_acq_rel,
       memory_order_acq_rel, memory_order_acq_rel, memory_order_seq_cst},
      {memory_order_seq_cst, memory_order_seq_cst, memory_order_seq_cst,
       memory_order_seq_cst, memory_order_seq_cst, memory_order_seq_cst}};

  //_Check_memory_order(_Success);
  check_load_memory_order<on_failure>();
  return COMBINED[static_cast<underlying_type_t<memory_order>>(on_success)]
                 [static_cast<underlying_type_t<memory_order>>(on_failure)];
}

// FUNCTION TEMPLATE atomic_reinterpret_as
template <class IntegralTy, class Ty>
[[nodiscard]] IntegralTy atomic_reinterpret_as(const Ty& source) noexcept {
  static_assert(is_integral_v<IntegralTy>,
                "tried to reinterpret memory as non-integral");
  if constexpr (is_integral_v<Ty> && sizeof(IntegralTy) == sizeof(Ty)) {
    return static_cast<IntegralTy>(source);
  } else if constexpr (is_pointer_v<Ty> && sizeof(IntegralTy) == sizeof(Ty)) {
    return reinterpret_cast<IntegralTy>(source);
  } else {
    return unaligned_load<IntegralTy>(addressof(source));
  }
}

template <memory_order order>
void make_load_barrier() noexcept {
  check_load_memory_order<order>();
  if constexpr (order != memory_order::relaxed) {
    make_compiler_barrier();
  }
}

template <memory_order order>
void make_store_barrier() noexcept {
  check_store_memory_order<order>();
  if constexpr (order != memory_order::relaxed) {
    make_compiler_barrier();
  }
}

template <class Ty>
struct atomic_padded {
  alignas(sizeof(Ty)) mutable Ty value;  // align to sizeof(T); x86 stack aligns
                                         // 8-byte objects on 4-byte boundaries
};

template <class Ty>
struct atomic_storage_selector {
  using storage_type = atomic_padded<Ty>;
  using lock_type = spin_lock;
};

template <class Ty>
struct atomic_storage_selector<Ty&> {
  using storage_type = Ty&;
  using lock_type = spin_lock;
};

template <class Ty, size_t = sizeof(remove_reference_t<Ty>)>
struct atomic_storage;

template <class Ty, size_t /* = ... */>
struct atomic_storage {
  // Provides operations common to all specializations of std::atomic, load,
  // store, exchange, and CAS. Locking version used when hardware has no atomic
  // operations for sizeof(Ty).

 public:
  using value_type = remove_reference_t<Ty>;

 private:
  using selector = atomic_storage_selector<Ty>;
  using storage_t = typename selector::storage_t;
  using lock_t = typename selector::lock_t;
  using guard_t = auto_lock<lock_t>;

 public:
  atomic_storage() noexcept(is_nothrow_default_constructible_v<Ty>) = default;

  constexpr atomic_storage(
      conditional_t<is_reference_v<Ty>, Ty, const value_type> value) noexcept
      : m_value{value} {
    // non-atomically initialize this atomic
  }

  template <memory_order order = memory_order_seq_cst>
  void store(const value_type value) noexcept {
    guard_t guard{m_lock};
    m_value = value;
  }

  template <memory_order order = memory_order_seq_cst>
  [[nodiscard]] value_type load() const noexcept {
    // load with sequential consistency
    guard_t guard{m_lock};
    value_type local_copy{m_value};
    return local_copy;
  }

  template <memory_order order = memory_order_seq_cst>
  value_type exchange(const value_type new_value) noexcept {
    guard_t guard{m_lock};
    value_type old_value{m_value};
    m_value = new_value;
    return old_value;
  }

  template <memory_order order = memory_order_seq_cst>
  bool compare_exchange_strong(value_type& expected,
                               const value_type desired) noexcept {  // CAS
    const auto* target_ptr = addressof(m_value);
    const auto* expected_ptr = addressof(expected);
    bool matched;

    guard_t guard{m_lock};
    matched = memcmp(target_ptr, expected_ptr, sizeof(value_type)) == 0;
    if (matched) {
      memcpy(target_ptr, addressof(desired), sizeof(value_type));
    } else {
      memcpy(expected_ptr, target_ptr, sizeof(value_type));
    }
    return matched;
  }

 public:
  storage_t m_value{};

 private:
  mutable lock_t m_lock{};
};

template <class Ty, class InterlockedPolicy>
class interlocked_storage {
 public:
  using value_type = Ty;

 private:
  using internal_value_type = typename InterlockedPolicy::value_type;
  using storage_type =
      typename atomic_storage_selector<value_type>::storage_type;

 public:
  template <memory_order order = memory_order_seq_cst>
  value_type load() noexcept {
    internal_value_type result{*get_storage()};
    th::details::make_load_barrier<order>();
    return reinterpret_cast<ValueTy&>(result);
  }

  template <memory_order order = memory_order_seq_cst>
  void store(const value_type value) noexcept {
    const auto source{atomic_reinterpret_as<internal_value_type>(value)};
    auto* place{get_storage()};
    th::details::make_store_barrier<order>();
    *place = source;
  }

  template <>
  void store<memory_order::seq_cst>(const value_type value) noexcept {
    [[maybe_unused]] auto old_value{exchange(value)};
  }

  template <memory_order order = memory_order_seq_cst>
  value_type exchange(const value_type value) noexcept {
    const auto new_value{atomic_reinterpret_as<internal_value_type>(value)};
    internal_value_type old_value{
        InterlockedPolicy::exchange(get_storage(), new_value)};
    return reinterpret_cast<value_type&>(old_value);
  }

  template <memory_order order = memory_order_seq_cst>
  bool compare_exchange_strong(value_type& expected,
                               const value_type desired) noexcept {
    const auto expected_bytes{atomic_reinterpret_as<internal_value_type>(
        expected)};  // read before atomic
    const auto desired_bytes{
        atomic_reinterpret_as<internal_value_type>(desired)};

    const auto old_value{InterlockedPolicy::compare_exchange_strong(
        get_storage(), desired_bytes, expected_bytes)};

    if (old_value == expected_bytes) {
      return true;
    }
    reinterpret_cast<internal_value_type&>(expected) = old_value;
    return false;
  }

 protected:
  auto* get_storage() noexcept {
    return atomic_address_as<internal_value_type>(m_value);
  }

  auto* get_storage() const noexcept {
    return ñonst_cast<internal_value_type*>(
        atomic_address_as<const internal_value_type>(m_value));
  }

 public:
  storage_type m_value;
};

template <size_t>
struct interlocked_policy;

template <>
struct interlocked_policy<32> {
  using value_type = long;

  static long exchange(volatile long* place, long new_value) noexcept {
    return InterlockedExchange(place, new_value);
  }

  static long compare_exchange_strong(volatile long* place,
                                      long desired,
                                      long expected) noexcept {
    return InterlockedCompareExchange(place, desired, expected);
  }
};

#define DEFINE_INTERLOCKED_POLICY(type, bitness)                              \
  template <>                                                                 \
  struct interlocked_policy<bitness> {                                        \
    using value_type = type;                                                  \
                                                                              \
    static type exchange(volatile type* place,                                \
                         value_type new_value) noexcept {                     \
      return InterlockedExchange##bitness##(place, new_value);                \
    }                                                                         \
                                                                              \
    static type compare_exchange_strong(volatile type* place,                 \
                                        type desired,                         \
                                        type expected) noexcept {             \
      return InterlockedCompareExchange##bitness##(place, desired, expected); \
    }                                                                         \
  };

DEFINE_INTERLOCKED_POLICY(char, 8)
DEFINE_INTERLOCKED_POLICY(short, 16)
DEFINE_INTERLOCKED_POLICY(long long, 64)

#undef DEFINE_INTERLOCKED_POLICY

#define DEFINE_ATOMIC_STORAGE(sizeoftype)                                    \
  template <class Ty>                                                        \
  struct atomic_storage<Ty, sizeoftype>                                      \
      : interlocked_storage<Ty, interlocked_policy<sizeoftype * CHAR_BIT>> { \
   public:                                                                   \
    using MyBase =                                                           \
        interlocked_storage<Ty, interlocked_policy<sizeoftype * CHAR_BIT>>;  \
                                                                             \
   public:                                                                   \
    using MyBase::load;                                                      \
    using MyBase::store;                                                     \
    using MyBase::exchange;                                                  \
    using MyBase::compare_exchange_strong;                                   \
  };

DEFINE_ATOMIC_STORAGE(1)
DEFINE_ATOMIC_STORAGE(2)
DEFINE_ATOMIC_STORAGE(4)
DEFINE_ATOMIC_STORAGE(8)

#undef DEFINE_ATOMIC_STORAGE

template <class Ty, size_t = sizeof(Ty)>
struct atomic_integral;  // not defined

template <class Ty, class IntegralPolicy>
class interlocked_integral : public atomic_storage<Ty> {
 public:
  using MyBase = atomic_storage<Ty>;

  using value_type = Ty;

 private:
  using internal_value_type = typename IntegralPolicy::value_type;

 public:
  template <memory_order order = memory_order_seq_cst>
  value_type fetch_add(const value_type operand) noexcept {
    return static_cast<value_type>(IntegralPolicy::add(
        get_storage(), static_cast<internal_value_type>(operand)));
  }

  template <memory_order order = memory_order_seq_cst>
  value_type fetch_and(const value_type operand) noexcept {
    return static_cast<value_type>(IntegralPolicy::and(
        get_storage(), static_cast<internal_value_type>(operand)));
  }

  template <memory_order order = memory_order_seq_cst>
  value_type fetch_or(const value_type operand) noexcept {
    return static_cast<value_type>(
        IntegralPolicy:: or
        (get_storage(), static_cast<internal_value_type>(operand)));
  }

  template <memory_order order = memory_order_seq_cst>
  value_type fetch_xor(const value_type operand) noexcept {
    return static_cast<value_type>(
        IntegralPolicy:: xor
        (get_storage(), static_cast<internal_value_type>(operand)));
  }

  value_type operator++() noexcept {
    return static_cast<value_type>(
        IntegralPolicy::pre_increment(get_storage()));
  }

  value_type operator++(int) noexcept {
    return static_cast<value_type>(
        IntegralPolicy::post_increment(get_storage()));
  }

  value_type operator--() noexcept {
    return static_cast<value_type>(
        IntegralPolicy::pre_decrement(get_storage()));
  }

  value_type operator--(int) noexcept {
    return static_cast<value_type>(
        IntegralPolicy::post_decrement(get_storage()));
  }

 protected:
  using MyBase::get_storage;
};

template <size_t>
struct integral_policy;

template <>
struct integral_policy<32> {
  using value_type = long;

  static long add(volatile long* addend, long value) noexcept {
    return InterlockedAdd(addend, value) - value;
  }

  static long and (volatile long* place, long value) noexcept {
    return InterlockedAnd(place, value);
  }

  static long or (volatile long* place, long value) noexcept {
    return InterlockedOr(place, value);
  }

  static long xor (volatile long* place, long value) noexcept {
    return InterlockedXor(place, value);
  }

  static long pre_increment(volatile long* addend) noexcept {
    return InterlockedIncrement(addend);
  }

  static long post_increment(volatile long* addend) noexcept {
    return InterlockedIncrement(addend) - 1;
  }

  static long pre_decrement(volatile long* addend) noexcept {
    return InterlockedDecrement(addend);
  }

  static long post_decrement(volatile long* addend) noexcept {
    return InterlockedDecrement(addend) + 1;
  }
};

#define DEFINE_INTEGRAL_POLICY(type, bitness)                      \
  template <>                                                      \
  struct integral_policy<bitness> {                                \
    using value_type = type;                                       \
                                                                   \
    static type add(volatile type* addend, type value) noexcept {  \
      return InterlockedAdd##bitness(addend, value) - value;       \
    }                                                              \
                                                                   \
    static type and (volatile type * place, type value) noexcept { \
      return InterlockedAnd##bitness(place, value);                \
    }                                                              \
                                                                   \
    static type or (volatile type * place, type value) noexcept {  \
      return InterlockedOr##bitness(place, value);                 \
    }                                                              \
                                                                   \
    static type xor (volatile type * place, type value) noexcept { \
      return InterlockedXor##bitness(place, value);                \
    }                                                              \
                                                                   \
    static type pre_increment(volatile type* addend) noexcept {    \
      return InterlockedIncrement##bitness(addend);                \
    }                                                              \
                                                                   \
    static type post_increment(volatile type* addend) noexcept {   \
      return InterlockedIncrement##bitness(addend) - 1;            \
    }                                                              \
                                                                   \
    static type pre_decrement(volatile type* addend) noexcept {    \
      return InterlockedDecrement##bitness(addend);                \
    }                                                              \
                                                                   \
    static type post_decrement(volatile type* addend) noexcept {   \
      return InterlockedDecrement##bitness(addend) + 1;            \
    }                                                              \
  };

DEFINE_INTEGRAL_POLICY(char, 8)
DEFINE_INTEGRAL_POLICY(short, 16)
DEFINE_INTEGRAL_POLICY(long long, 64)

#undef DEFINE_INTEGRAL_POLICY

template <class Ty, size_t>
struct integral_storage;

#define DEFINE_INTEGRAL_STORAGE(sizeoftype)                                \
  template <class Ty>                                                      \
  struct integral_storage<Ty, sizeoftype>                                  \
      : interlocked_integral<Ty, integral_policy<sizeoftype * CHAR_BIT>> { \
   public:                                                                 \
    using MyBase =                                                         \
        interlocked_integral<Ty, integral_policy<sizeoftype * CHAR_BIT>>;  \
                                                                           \
   public:                                                                 \
    using MyBase::fetch_add;                                               \
    using MyBase::fetch_and;                                               \
    using MyBase::fetch_or;                                                \
    using MyBase::fetch_xor;                                               \
    using MyBase::operator++;                                              \
    using MyBase::operator--;                                              \
  };

DEFINE_INTEGRAL_STORAGE(1)
DEFINE_INTEGRAL_STORAGE(2)
DEFINE_INTEGRAL_STORAGE(4)
DEFINE_INTEGRAL_STORAGE(8)

#undef DEFINE_INTEGRAL_STORAGE

template <class Ty>
struct is_always_lock_free {
  using value_type = Ty;
  static constexpr size_t SIZE_OF_TYPE{sizeof(Ty)};

  static constexpr bool value = SIZE_OF_TYPE <= sizeof(uintmax_t) &&
                                (SIZE_OF_TYPE & SIZE_OF_TYPE - 1) == 0;
};

template <class Ty>
inline constexpr bool is_always_lock_free_v = is_always_lock_free<Ty>::value;

template <class Ty>
struct integral_facade : integral_storage<Ty, sizeof(Ty)> {
  using MyBase = integral_storage<Ty, sizeof(Ty)>;
  using difference_type = Ty;

  using MyBase::fetch_add;
  Ty fetch_add(const Ty operand) volatile noexcept {
    return const_cast<integral_facade*>(this)->MyBase::fetch_add(operand);
  }

  Ty fetch_add(const Ty operand, const memory_order _Order) volatile noexcept {
    return const_cast<integral_facade*>(this)->MyBase::fetch_add(operand,
                                                                 _Order);
  }

  Ty fetch_sub(const Ty operand) noexcept { return fetch_add(negate(operand)); }

  Ty fetch_sub(const Ty operand) volatile noexcept {
    return fetch_add(negate(operand));
  }

  Ty fetch_sub(const Ty operand, const memory_order _Order) noexcept {
    return fetch_add(negate(operand), _Order);
  }

  Ty fetch_sub(const Ty operand, const memory_order _Order) volatile noexcept {
    return fetch_add(negate(operand), _Order);
  }

  using MyBase::fetch_and;
  Ty fetch_and(const Ty operand) volatile noexcept {
    return const_cast<integral_facade*>(this)->MyBase::fetch_and(operand);
  }

  Ty fetch_and(const Ty operand, const memory_order _Order) volatile noexcept {
    return const_cast<integral_facade*>(this)->MyBase::fetch_and(operand,
                                                                 _Order);
  }

  using MyBase::fetch_or;
  Ty fetch_or(const Ty operand) volatile noexcept {
    return const_cast<integral_facade*>(this)->MyBase::fetch_or(operand);
  }

  Ty fetch_or(const Ty operand, const memory_order _Order) volatile noexcept {
    return const_cast<integral_facade*>(this)->MyBase::fetch_or(operand,
                                                                _Order);
  }

  using MyBase::fetch_xor;
  Ty fetch_xor(const Ty operand) volatile noexcept {
    return const_cast<integral_facade*>(this)->MyBase::fetch_xor(operand);
  }

  Ty fetch_xor(const Ty operand, const memory_order _Order) volatile noexcept {
    return const_cast<integral_facade*>(this)->MyBase::fetch_xor(operand,
                                                                 _Order);
  }

  using MyBase::operator++;
  Ty operator++(int) volatile noexcept {
    return const_cast<integral_facade*>(this)->MyBase::operator++(0);
  }

  Ty operator++() volatile noexcept {
    return const_cast<integral_facade*>(this)->MyBase::operator++();
  }

  using MyBase::operator--;
  Ty operator--(int) volatile noexcept {
    return const_cast<integral_facade*>(this)->MyBase::operator--(0);
  }

  Ty operator--() volatile noexcept {
    return const_cast<integral_facade*>(this)->MyBase::operator--();
  }

  Ty operator+=(const Ty operand) noexcept {
    return static_cast<Ty>(this->MyBase::fetch_add(operand) + operand);
  }

  Ty operator+=(const Ty operand) volatile noexcept {
    return static_cast<Ty>(
        const_cast<integral_facade*>(this)->MyBase::fetch_add(operand) +
        operand);
  }

  Ty operator-=(const Ty operand) noexcept {
    return static_cast<Ty>(fetch_sub(operand) - operand);
  }

  Ty operator-=(const Ty operand) volatile noexcept {
    return static_cast<Ty>(
        const_cast<integral_facade*>(this)->fetch_sub(operand) - operand);
  }

  Ty operator&=(const Ty operand) noexcept {
    return static_cast<Ty>(this->MyBase::fetch_and(operand) & operand);
  }

  Ty operator&=(const Ty operand) volatile noexcept {
    return static_cast<Ty>(
        const_cast<integral_facade*>(this)->MyBase::fetch_and(operand) &
        operand);
  }

  Ty operator|=(const Ty operand) noexcept {
    return static_cast<Ty>(this->MyBase::fetch_or(operand) | operand);
  }

  Ty operator|=(const Ty operand) volatile noexcept {
    return static_cast<Ty>(
        const_cast<integral_facade*>(this)->MyBase::fetch_or(operand) |
        operand);
  }

  Ty operator^=(const Ty operand) noexcept {
    return static_cast<Ty>(this->MyBase::fetch_xor(operand) ^ operand);
  }

  Ty operator^=(const Ty operand) volatile noexcept {
    return static_cast<Ty>(
        const_cast<integral_facade*>(this)->MyBase::fetch_xor(operand) ^
        operand);
  }

  [[nodiscard]] static Ty negate(
      const Ty value) noexcept {  // returns two's complement negated value of
                                  // value
    return static_cast<Ty>(0U - static_cast<make_unsigned_t<Ty>>(value));
  }
};

//// STRUCT TEMPLATE atomic_integral_facade
// template <class Ty>
// struct atomic_integral_facade<Ty&> : atomic_integral<Ty&> {
//  // provides operator overloads and other support for atomic integral
//  // specializations
//  using MyBase = atomic_integral<Ty&>;
//  using difference_type = Ty;
//
//  using MyBase::MyBase;
//
//  [[nodiscard]] static Ty _Negate(
//      const Ty value) noexcept {  // returns two's complement negated value of
//                                  // value
//    return static_cast<Ty>(0U - static_cast<make_unsigned_t<Ty>>(value));
//  }
//
//  Ty fetch_add(const Ty operand) const noexcept {
//    return const_cast<atomic_integral_facade*>(this)->MyBase::fetch_add(
//        operand);
//  }
//
//  Ty fetch_add(const Ty operand, const memory_order _Order) const noexcept {
//    return
//    const_cast<atomic_integral_facade*>(this)->MyBase::fetch_add(operand,
//                                                                        _Order);
//  }
//
//  Ty fetch_sub(const Ty operand) const noexcept {
//    return fetch_add(_Negate(operand));
//  }
//
//  Ty fetch_sub(const Ty operand, const memory_order _Order) const noexcept {
//    return fetch_add(_Negate(operand), _Order);
//  }
//
//  Ty operator+=(const Ty operand) const noexcept {
//    return static_cast<Ty>(fetch_add(operand) + operand);
//  }
//
//  Ty operator-=(const Ty operand) const noexcept {
//    return static_cast<Ty>(fetch_sub(operand) - operand);
//  }
//
//  Ty fetch_and(const Ty operand) const noexcept {
//    return const_cast<atomic_integral_facade*>(this)->MyBase::fetch_and(
//        operand);
//  }
//
//  Ty fetch_and(const Ty operand, const memory_order _Order) const noexcept {
//    return
//    const_cast<atomic_integral_facade*>(this)->MyBase::fetch_and(operand,
//                                                                        _Order);
//  }
//
//  Ty fetch_or(const Ty operand) const noexcept {
//    return
//    const_cast<atomic_integral_facade*>(this)->MyBase::fetch_or(operand);
//  }
//
//  Ty fetch_or(const Ty operand, const memory_order _Order) const noexcept {
//    return
//    const_cast<atomic_integral_facade*>(this)->MyBase::fetch_or(operand,
//                                                                       _Order);
//  }
//
//  Ty fetch_xor(const Ty operand) const noexcept {
//    return const_cast<atomic_integral_facade*>(this)->MyBase::fetch_xor(
//        operand);
//  }
//
//  Ty fetch_xor(const Ty operand, const memory_order _Order) const noexcept {
//    return
//    const_cast<atomic_integral_facade*>(this)->MyBase::fetch_xor(operand,
//                                                                        _Order);
//  }
//
//  Ty operator&=(const Ty operand) const noexcept {
//    return static_cast<Ty>(fetch_and(operand) & operand);
//  }
//
//  Ty operator|=(const Ty operand) const noexcept {
//    return static_cast<Ty>(fetch_or(operand) | operand);
//  }
//
//  Ty operator^=(const Ty operand) const noexcept {
//    return static_cast<Ty>(fetch_xor(operand) ^ operand);
//  }
//};

template <class Ty>
struct atomic_floating;

// template <class Ty>
// struct atomic_floating : atomic_storage<Ty> {
//  // provides atomic floating-point operations
//  using MyBase = atomic_storage<Ty>;
//  using difference_type = Ty;
//
//  using MyBase::MyBase;
//
//  Ty fetch_add(const Ty operand,
//               const memory_order _Order = memory_order_seq_cst) noexcept {
//    Ty _Temp{this->load(memory_order_relaxed)};
//    while (!this->compare_exchange_strong(_Temp, _Temp + operand,
//                                          _Order)) {  // keep trying
//    }
//
//    return _Temp;
//  }
//
//  // _Deprecate_non_lock_free_volatile is unnecessary here.
//
//  // note: const_cast-ing away volatile is safe because all our intrinsics add
//  // volatile back on. We make the primary functions non-volatile for better
//  // debug codegen, as non-volatile atomics are far more common than volatile
//  // ones.
//  Ty fetch_add(
//      const Ty operand,
//      const memory_order _Order = memory_order_seq_cst) volatile noexcept {
//    return const_cast<atomic_floating*>(this)->fetch_add(operand, _Order);
//  }
//
//  Ty fetch_sub(const Ty operand,
//               const memory_order _Order = memory_order_seq_cst) noexcept {
//    Ty _Temp{this->load(memory_order_relaxed)};
//    while (!this->compare_exchange_strong(_Temp, _Temp - operand,
//                                          _Order)) {  // keep trying
//    }
//
//    return _Temp;
//  }
//
//  Ty fetch_sub(
//      const Ty operand,
//      const memory_order _Order = memory_order_seq_cst) volatile noexcept {
//    return const_cast<atomic_floating*>(this)->fetch_sub(operand, _Order);
//  }
//
//  Ty operator+=(const Ty operand) noexcept {
//    return fetch_add(operand) + operand;
//  }
//
//  Ty operator+=(const Ty operand) volatile noexcept {
//    return const_cast<atomic_floating*>(this)->fetch_add(operand) + operand;
//  }
//
//  Ty operator-=(const Ty operand) noexcept {
//    return fetch_sub(operand) - operand;
//  }
//
//  Ty operator-=(const Ty operand) volatile noexcept {
//    return const_cast<atomic_floating*>(this)->fetch_sub(operand) - operand;
//  }
//};
//
// template <class Ty>
// struct atomic_floating<Ty&> : atomic_storage<Ty&> {
//  // provides atomic floating-point operations
//  using MyBase = atomic_storage<Ty&>;
//  using difference_type = Ty;
//
//  using MyBase::MyBase;
//
//  Ty fetch_add(
//      const Ty operand,
//      const memory_order _Order = memory_order_seq_cst) const noexcept {
//    Ty _Temp{this->load(memory_order_relaxed)};
//    while
//    (!const_cast<atomic_floating*>(this)->MyBase::compare_exchange_strong(
//        _Temp, _Temp + operand, _Order)) {  // keep trying
//    }
//
//    return _Temp;
//  }
//
//  Ty fetch_sub(
//      const Ty operand,
//      const memory_order _Order = memory_order_seq_cst) const noexcept {
//    Ty _Temp{this->load(memory_order_relaxed)};
//    while
//    (!const_cast<atomic_floating*>(this)->MyBase::compare_exchange_strong(
//        _Temp, _Temp - operand, _Order)) {  // keep trying
//    }
//
//    return _Temp;
//  }
//
//  Ty operator+=(const Ty operand) const noexcept {
//    return fetch_add(operand) + operand;
//  }
//
//  Ty operator-=(const Ty operand) const noexcept {
//    return fetch_sub(operand) - operand;
//  }
//};

template <class Ty>
struct atomic_pointer;

//// STRUCT TEMPLATE atomic_pointer
// template <class Ty>
// struct atomic_pointer : atomic_storage<Ty> {
//  using MyBase = atomic_storage<Ty>;
//  using difference_type = ptrdiff_t;
//
//  using MyBase::MyBase;
//
//  Ty fetch_add(const ptrdiff_t _Diff,
//               const memory_order _Order = memory_order_seq_cst) noexcept {
//    const ptrdiff_t _Shift_bytes = static_cast<ptrdiff_t>(
//        static_cast<size_t>(_Diff) * sizeof(remove_pointer_t<Ty>));
//    ptrdiff_t _Result;
//#if defined(_M_IX86) || defined(_M_ARM)
//    ATOMIC_INTRINSIC(_Order, _Result, _InterlockedExchangeAdd,
//                     atomic_address_as<long>(this->_Storage), _Shift_bytes);
//#else   // ^^^ 32 bits / 64 bits vvv
//    ATOMIC_INTRINSIC(_Order, _Result, _InterlockedExchangeAdd64,
//                     atomic_address_as<long long>(this->_Storage),
//                     _Shift_bytes);
//#endif  // hardware
//    return reinterpret_cast<Ty>(_Result);
//  }
//
//  // _Deprecate_non_lock_free_volatile is unnecessary here.
//
//  Ty fetch_add(const ptrdiff_t _Diff) volatile noexcept {
//    return const_cast<atomic_pointer*>(this)->fetch_add(_Diff);
//  }
//
//  Ty fetch_add(const ptrdiff_t _Diff,
//               const memory_order _Order) volatile noexcept {
//    return const_cast<atomic_pointer*>(this)->fetch_add(_Diff, _Order);
//  }
//
//  Ty fetch_sub(const ptrdiff_t _Diff) volatile noexcept {
//    return fetch_add(static_cast<ptrdiff_t>(0 - static_cast<size_t>(_Diff)));
//  }
//
//  Ty fetch_sub(const ptrdiff_t _Diff) noexcept {
//    return fetch_add(static_cast<ptrdiff_t>(0 - static_cast<size_t>(_Diff)));
//  }
//
//  Ty fetch_sub(const ptrdiff_t _Diff,
//               const memory_order _Order) volatile noexcept {
//    return fetch_add(static_cast<ptrdiff_t>(0 - static_cast<size_t>(_Diff)),
//                     _Order);
//  }
//
//  Ty fetch_sub(const ptrdiff_t _Diff, const memory_order _Order) noexcept {
//    return fetch_add(static_cast<ptrdiff_t>(0 - static_cast<size_t>(_Diff)),
//                     _Order);
//  }
//
//  Ty operator++(int) volatile noexcept { return fetch_add(1); }
//
//  Ty operator++(int) noexcept { return fetch_add(1); }
//
//  Ty operator++() volatile noexcept { return fetch_add(1) + 1; }
//
//  Ty operator++() noexcept { return fetch_add(1) + 1; }
//
//  Ty operator--(int) volatile noexcept { return fetch_add(-1); }
//
//  Ty operator--(int) noexcept { return fetch_add(-1); }
//
//  Ty operator--() volatile noexcept { return fetch_add(-1) - 1; }
//
//  Ty operator--() noexcept { return fetch_add(-1) - 1; }
//
//  Ty operator+=(const ptrdiff_t _Diff) volatile noexcept {
//    return fetch_add(_Diff) + _Diff;
//  }
//
//  Ty operator+=(const ptrdiff_t _Diff) noexcept {
//    return fetch_add(_Diff) + _Diff;
//  }
//
//  Ty operator-=(const ptrdiff_t _Diff) volatile noexcept {
//    return fetch_add(static_cast<ptrdiff_t>(0 - static_cast<size_t>(_Diff))) -
//           _Diff;
//  }
//
//  Ty operator-=(const ptrdiff_t _Diff) noexcept {
//    return fetch_add(static_cast<ptrdiff_t>(0 - static_cast<size_t>(_Diff))) -
//           _Diff;
//  }
//};
//
//// STRUCT TEMPLATE atomic_pointer
// template <class Ty>
// struct atomic_pointer<Ty&> : atomic_storage<Ty&> {
//  using MyBase = atomic_storage<Ty&>;
//  using difference_type = ptrdiff_t;
//
//  using MyBase::MyBase;
//
//  Ty fetch_add(
//      const ptrdiff_t _Diff,
//      const memory_order _Order = memory_order_seq_cst) const noexcept {
//    const ptrdiff_t _Shift_bytes = static_cast<ptrdiff_t>(
//        static_cast<size_t>(_Diff) * sizeof(remove_pointer_t<Ty>));
//    ptrdiff_t _Result;
//#if defined(_M_IX86) || defined(_M_ARM)
//    ATOMIC_INTRINSIC(_Order, _Result, _InterlockedExchangeAdd,
//                     atomic_address_as<long>(this->_Storage), _Shift_bytes);
//#else   // ^^^ 32 bits / 64 bits vvv
//    ATOMIC_INTRINSIC(_Order, _Result, _InterlockedExchangeAdd64,
//                     atomic_address_as<long long>(this->_Storage),
//                     _Shift_bytes);
//#endif  // hardware
//    return reinterpret_cast<Ty>(_Result);
//  }
//
//  Ty fetch_sub(const ptrdiff_t _Diff) const noexcept {
//    return fetch_add(static_cast<ptrdiff_t>(0 - static_cast<size_t>(_Diff)));
//  }
//
//  Ty fetch_sub(const ptrdiff_t _Diff,
//               const memory_order _Order) const noexcept {
//    return fetch_add(static_cast<ptrdiff_t>(0 - static_cast<size_t>(_Diff)),
//                     _Order);
//  }
//
//  Ty operator++(int) const noexcept { return fetch_add(1); }
//
//  Ty operator++() const noexcept { return fetch_add(1) + 1; }
//
//  Ty operator--(int) const noexcept { return fetch_add(-1); }
//
//  Ty operator--() const noexcept { return fetch_add(-1) - 1; }
//
//  Ty operator+=(const ptrdiff_t _Diff) const noexcept {
//    return fetch_add(_Diff) + _Diff;
//  }
//
//  Ty operator-=(const ptrdiff_t _Diff) const noexcept {
//    return fetch_add(static_cast<ptrdiff_t>(0 - static_cast<size_t>(_Diff))) -
//           _Diff;
//  }
//};
//

}  // namespace th::details

template <bool Cond>
struct atomic_template_selector {
  template <class Ty1, class>
  using type = Ty1;
};

template <>
struct atomic_template_selector<false> {
  template <class, class Ty2>
  using type = Ty2;
};

template <class Ty, class ValueTy = Ty>
struct atomic_base_type_selector {
  using type =
      typename atomic_template_selector<is_floating_point_v<Ty>>::template type<
          th::details::atomic_floating<ValueTy>,
          typename atomic_template_selector<is_integral_v<Ty> &&
                                            !is_same_v<bool, Ty>>::
              template type<
                  th::details::integral_facade<ValueTy>,
                  typename atomic_template_selector<
                      is_pointer_v<Ty> && is_object_v<remove_pointer_t<Ty>>>::
                      template type<th::details::atomic_pointer<ValueTy>,
                                    th::details::atomic_storage<ValueTy>>>>;
};

template <class Ty>
using atomic_base_t = typename atomic_base_type_selector<Ty>::type;

// template <class _TVal, class _Ty = _TVal>
// using atomic_base_t =
//    typename _Select<>::template _Apply<atomic_floating<_Ty>,
//                                        _Choose_atomic_base2_t<_TVal, _Ty>>;

template <class Ty>
class atomic : public atomic_base_t<Ty> {  // atomic value
 private:
  using MyBase = atomic_base_t<Ty>;

 public:
  using value_type = Ty;

 public:
  static constexpr bool is_always_lock_free =
      th::details::is_always_lock_free_v<Ty>;

 public:
  static_assert(
      is_trivially_copyable_v<Ty> && is_copy_constructible_v<Ty> &&
          is_move_constructible_v<Ty> && is_copy_assignable_v<Ty> &&
          is_move_assignable_v<Ty>,
      "atomic<Ty> requires T to be trivially copyable, copy constructible, "
      "move constructible, copy assignable, and move assignable");

 public:
  constexpr atomic() noexcept(is_nothrow_default_constructible_v<Ty>) = default;

  atomic(const atomic&) = delete;
  atomic& operator=(const atomic&) = delete;

  [[nodiscard]] bool is_lock_free() const volatile noexcept {
    return is_always_lock_free_v<Ty>;
  }

  [[nodiscard]] bool is_lock_free() const noexcept {
    return static_cast<const volatile atomic*>(this)->is_lock_free();
  }

  Ty operator=(const Ty value) volatile noexcept {
    MyBase::store(value);
    return value;
  }

  Ty operator=(const Ty value) noexcept {
    MyBase::store(value);
    return value;
  }

  using MyBase::store;

  void store(const Ty value) volatile noexcept {
    const_cast<atomic*>(this)->MyBase::store(value);
  }

  template <memory_order order>
  void store(const Ty value) volatile noexcept {
    const_cast<atomic*>(this)->MyBase::store<order>(value);
  }

  using MyBase::load;
  [[nodiscard]] Ty load() const volatile noexcept {
    return const_cast<const atomic*>(this)->MyBase::load();
  }

  template <memory_order order>
  [[nodiscard]] Ty load() const volatile noexcept {
    return const_cast<const atomic*>(this)->MyBase::load<order>();
  }

  using MyBase::exchange;
  Ty exchange(const Ty value) volatile noexcept {
    return const_cast<atomic*>(this)->MyBase::exchange(value);
  }

  template <memory_order order>
  Ty exchange(const Ty value) volatile noexcept {
    return const_cast<atomic*>(this)->MyBase::exchange<order>(value);
  }

  using MyBase::compare_exchange_strong;

  bool compare_exchange_strong(Ty& expected,
                               const Ty desired) volatile noexcept {
    return const_cast<atomic*>(this)->MyBase::compare_exchange_strong(expected,
                                                                      desired);
  }

  template <memory_order order>
  bool compare_exchange_strong(Ty& expected,
                               const Ty desired) volatile noexcept {
    return const_cast<atomic*>(this)->MyBase::compare_exchange_strong<order>(
        expected, desired);
  }

  template <memory_order on_success, memory_order on_failure>
  bool compare_exchange_strong(Ty& expected,
                               const Ty desired) volatile noexcept {
    return MyBase::compare_exchange_strong<
        combine_cas_memory_orders<on_success, on_failure>>(expected, desired);
  }

  template <memory_order on_success, memory_order on_failure>
  bool compare_exchange_strong(Ty& expected,
                               const Ty desired,
                               const memory_order _Success,
                               const memory_order _Failure) noexcept {
    return MyBase::compare_exchange_strong<
        combine_cas_memory_orders<on_success, on_failure>>(expected, desired);
  }

  bool compare_exchange_weak(Ty& expected, const Ty desired) volatile noexcept {
    // MSVC have no weak CAS intrinsics, even on ARM32/ARM64, so fall back
    // to strong

    return MyBase::compare_exchange_strong(expected, desired);
  }

  bool compare_exchange_weak(Ty& expected, const Ty desired) noexcept {
    return MyBase::compare_exchange_strong(expected, desired);
  }

  template <memory_order order>
  bool compare_exchange_weak(Ty& expected, const Ty desired) volatile noexcept {
    return MyBase::compare_exchange_strong<order>(expected, desired);
  }

  template <memory_order order>
  bool compare_exchange_weak(Ty& expected, const Ty desired) noexcept {
    return MyBase::compare_exchange_strong<order>(expected, desired);
  }

  template <memory_order on_success, memory_order on_failure>
  bool compare_exchange_weak(Ty& expected, const Ty desired) volatile noexcept {
    return MyBase::compare_exchange_strong<
        combine_cas_memory_orders<on_success, on_failure>>(expected, desired);
  }

  template <memory_order on_success, memory_order on_failure>
  bool compare_exchange_weak(Ty& expected, const Ty desired) noexcept {
    return MyBase::compare_exchange_strong<
        combine_cas_memory_orders<on_success, on_failure>>(expected, desired);
  }

  operator Ty() const volatile noexcept { return load(); }
  operator Ty() const noexcept { return load(); }
};

template <class Ty>
atomic(Ty) -> atomic<Ty>;
//
// template <class Ty>
// struct atomic_ref : atomic_base_t<Ty, Ty&> {  // atomic reference
// private:
//  using MyBase = atomic_base_t<Ty, Ty&>;
//
// public:
//  static_assert(
//      is_trivially_copyable_v<Ty> && is_copy_constructible_v<Ty> &&
//          is_move_constructible_v<Ty> && is_copy_assignable_v<Ty> &&
//          is_move_assignable_v<Ty>,
//      "atomic_ref<T> requires T to be trivially copyable, copy constructible,
//      " "move constructible, copy assignable, and move assignable");
//
//  using value_type = Ty;
//
//  explicit atomic_ref(Ty& value) noexcept /* strengthened */ : MyBase(value) {
//    if constexpr (_Is_potentially_lock_free) {
//      _Check_alignment(value);
//    } else {
//      this->_Init_spinlock_for_ref();
//    }
//  }
//
//  atomic_ref(const atomic_ref&) noexcept = default;
//
//  atomic_ref& operator=(const atomic_ref&) = delete;
//
//  static constexpr bool is_always_lock_free =
//  _Is_always_lock_free<sizeof(Ty)>;
//
//  static constexpr bool _Is_potentially_lock_free =
//      sizeof(Ty) <= 2 * sizeof(void*) && (sizeof(Ty) & (sizeof(Ty) - 1)) == 0;
//
//  static constexpr size_t required_alignment =
//      _Is_potentially_lock_free ? sizeof(Ty) : alignof(Ty);
//
//  [[nodiscard]] bool is_lock_free() const noexcept {
//#if _ATOMIC_HAS_DCAS
//    return is_always_lock_free;
//#else   // ^^^ _ATOMIC_HAS_DCAS / !_ATOMIC_HAS_DCAS vvv
//    if constexpr (is_always_lock_free) {
//      return true;
//    } else {
//      return __std_atomic_has_cmpxchg16b() != 0;
//    }
//#endif  // _ATOMIC_HAS_DCAS
//  }
//
//  void store(const Ty value) const noexcept {
//    const_cast<atomic_ref*>(this)->MyBase::store(value);
//  }
//
//  void store(const Ty value, const memory_order _Order) const noexcept {
//    const_cast<atomic_ref*>(this)->MyBase::store(value, _Order);
//  }
//
//  Ty operator=(const Ty value) const noexcept {
//    store(value);
//    return value;
//  }
//
//  Ty exchange(const Ty value) const noexcept {
//    return const_cast<atomic_ref*>(this)->MyBase::exchange(value);
//  }
//
//  Ty exchange(const Ty value, const memory_order _Order) const noexcept {
//    return const_cast<atomic_ref*>(this)->MyBase::exchange(value, _Order);
//  }
//
//  bool compare_exchange_strong(Ty& expected, const Ty desired) const noexcept
//  {
//    return const_cast<atomic_ref*>(this)->MyBase::compare_exchange_strong(
//        expected, desired);
//  }
//
//  bool compare_exchange_strong(Ty& expected,
//                               const Ty desired,
//                               const memory_order _Order) const noexcept {
//    return const_cast<atomic_ref*>(this)->MyBase::compare_exchange_strong(
//        expected, desired, _Order);
//  }
//
//  bool compare_exchange_strong(Ty& expected,
//                               const Ty desired,
//                               const memory_order _Success,
//                               const memory_order _Failure) const noexcept {
//    return compare_exchange_strong(
//        expected, desired, combine_cas_memory_orders(_Success, _Failure));
//  }
//
//  bool compare_exchange_weak(Ty& expected, const Ty desired) const noexcept {
//    return compare_exchange_strong(expected, desired);
//  }
//
//  bool compare_exchange_weak(Ty& expected,
//                             const Ty desired,
//                             const memory_order _Order) const noexcept {
//    return compare_exchange_strong(expected, desired, _Order);
//  }
//
//  bool compare_exchange_weak(Ty& expected,
//                             const Ty desired,
//                             const memory_order _Success,
//                             const memory_order _Failure) const noexcept {
//    return compare_exchange_strong(
//        expected, desired, combine_cas_memory_orders(_Success, _Failure));
//  }
//
//  operator Ty() const noexcept { return this->load(); }
//
//  void notify_one() const noexcept {
//    const_cast<atomic_ref*>(this)->MyBase::notify_one();
//  }
//
//  void notify_all() const noexcept {
//    const_cast<atomic_ref*>(this)->MyBase::notify_all();
//  }
//
// private:
//  static void _Check_alignment([[maybe_unused]] const Ty& value) {
//    _ATOMIC_REF_CHECK_ALIGNMENT(
//        (reinterpret_cast<uintptr_t>(addressof(value)) &
//         (required_alignment - 1)) == 0,
//        "atomic_ref underlying object is not aligned as required_alignment");
//  }
//};
//
//// NONMEMBER OPERATIONS ON ATOMIC TYPES
// template <class Ty>
//[[nodiscard]] bool atomic_is_lock_free(
//    const volatile atomic<Ty>* target) noexcept {
//  return target->is_lock_free();
//}
//
// template <class Ty>
//[[nodiscard]] bool atomic_is_lock_free(const atomic<Ty>* target) noexcept {
//  return target->is_lock_free();
//}
//
// template <class Ty>
// void atomic_store(volatile atomic<Ty>* const target,
//                  const identity_t<Ty> value) noexcept {
//  target->store(value);
//}
//
// template <class Ty>
// void atomic_store(atomic<Ty>* const target, const identity_t<Ty> value)
// noexcept {
//  target->store(value);
//}
//
// template <class Ty>
// void atomic_store_explicit(volatile atomic<Ty>* const target,
//                           const identity_t<Ty> value,
//                           const memory_order _Order) noexcept {
//  target->store(value, _Order);
//}
//
// template <class Ty>
// void atomic_store_explicit(atomic<Ty>* const target,
//                           const identity_t<Ty> value,
//                           const memory_order _Order) noexcept {
//  target->store(value, _Order);
//}
//
// template <class Ty>
//[[nodiscard]] Ty atomic_load(const volatile atomic<Ty>* const target) noexcept
//{
//  return target->load();
//}
//
// template <class Ty>
//[[nodiscard]] Ty atomic_load(const atomic<Ty>* const target) noexcept {
//  return target->load();
//}
//
// template <class Ty>
//[[nodiscard]] Ty atomic_load_explicit(const volatile atomic<Ty>* const target,
//                                      const memory_order _Order) noexcept {
//  return target->load(_Order);
//}
//
// template <class Ty>
//[[nodiscard]] Ty atomic_load_explicit(const atomic<Ty>* const target,
//                                      const memory_order _Order) noexcept {
//  return target->load(_Order);
//}
//
// template <class Ty>
// Ty atomic_exchange(volatile atomic<Ty>* const target,
//                   const identity_t<Ty> value) noexcept {
//  return target->exchange(value);
//}
//
// template <class Ty>
// Ty atomic_exchange(atomic<Ty>* const target,
//                   const identity_t<Ty> value) noexcept {
//  return target->exchange(value);
//}
//
// template <class Ty>
// Ty atomic_exchange_explicit(volatile atomic<Ty>* const target,
//                            const identity_t<Ty> value,
//                            const memory_order _Order) noexcept {
//  return target->exchange(value, _Order);
//}
//
// template <class Ty>
// Ty atomic_exchange_explicit(atomic<Ty>* const target,
//                            const identity_t<Ty> value,
//                            const memory_order _Order) noexcept {
//  return target->exchange(value, _Order);
//}
//
// template <class Ty>
// bool atomic_compare_exchange_strong(volatile atomic<Ty>* const target,
//                                    identity_t<Ty>* const expected,
//                                    const identity_t<Ty> desired) noexcept {
//  return target->compare_exchange_strong(*expected, desired);
//}
//
// template <class Ty>
// bool atomic_compare_exchange_strong(atomic<Ty>* const target,
//                                    identity_t<Ty>* const expected,
//                                    const identity_t<Ty> desired) noexcept {
//  return target->compare_exchange_strong(*expected, desired);
//}
//
// template <class Ty>
// bool atomic_compare_exchange_strong_explicit(
//    volatile atomic<Ty>* const target,
//    identity_t<Ty>* const expected,
//    const identity_t<Ty> desired,
//    const memory_order _Success,
//    const memory_order _Failure) noexcept {
//  return target->compare_exchange_strong(
//      *expected, desired, combine_cas_memory_orders(_Success, _Failure));
//}
//
// template <class Ty>
// bool atomic_compare_exchange_strong_explicit(
//    atomic<Ty>* const target,
//    identity_t<Ty>* const expected,
//    const identity_t<Ty> desired,
//    const memory_order _Success,
//    const memory_order _Failure) noexcept {
//  return target->compare_exchange_strong(
//      *expected, desired, combine_cas_memory_orders(_Success, _Failure));
//}
//
// template <class Ty>
// bool atomic_compare_exchange_weak(volatile atomic<Ty>* const target,
//                                  identity_t<Ty>* const expected,
//                                  const identity_t<Ty> desired) noexcept {
//  return target->compare_exchange_strong(*expected, desired);
//}
//
// template <class Ty>
// bool atomic_compare_exchange_weak(atomic<Ty>* const target,
//                                  identity_t<Ty>* const expected,
//                                  const identity_t<Ty> desired) noexcept {
//  return target->compare_exchange_strong(*expected, desired);
//}
//
// template <class Ty>
// bool atomic_compare_exchange_weak_explicit(
//    volatile atomic<Ty>* const target,
//    identity_t<Ty>* const expected,
//    const identity_t<Ty> desired,
//    const memory_order _Success,
//    const memory_order _Failure) noexcept {
//  return target->compare_exchange_strong(
//      *expected, desired, combine_cas_memory_orders(_Success, _Failure));
//}
//
// template <class Ty>
// bool atomic_compare_exchange_weak_explicit(
//    atomic<Ty>* const target,
//    identity_t<Ty>* const expected,
//    const identity_t<Ty> desired,
//    const memory_order _Success,
//    const memory_order _Failure) noexcept {
//  return target->compare_exchange_strong(
//      *expected, desired, combine_cas_memory_orders(_Success, _Failure));
//}
//
// template <class Ty>
// Ty atomic_fetch_add(volatile atomic<Ty>* target,
//                    const typename atomic<Ty>::difference_type value) noexcept
//                    {
//  return target->fetch_add(value);
//}
//
// template <class Ty>
// Ty atomic_fetch_add(atomic<Ty>* target,
//                    const typename atomic<Ty>::difference_type value) noexcept
//                    {
//  return target->fetch_add(value);
//}
//
// template <class Ty>
// Ty atomic_fetch_add_explicit(volatile atomic<Ty>* target,
//                             const typename atomic<Ty>::difference_type value,
//                             const memory_order _Order) noexcept {
//  return target->fetch_add(value, _Order);
//}
//
// template <class Ty>
// Ty atomic_fetch_add_explicit(atomic<Ty>* target,
//                             const typename atomic<Ty>::difference_type value,
//                             const memory_order _Order) noexcept {
//  return target->fetch_add(value, _Order);
//}
//
// template <class Ty>
// Ty atomic_fetch_sub(volatile atomic<Ty>* target,
//                    const typename atomic<Ty>::difference_type value) noexcept
//                    {
//  return target->fetch_sub(value);
//}
//
// template <class Ty>
// Ty atomic_fetch_sub(atomic<Ty>* target,
//                    const typename atomic<Ty>::difference_type value) noexcept
//                    {
//  return target->fetch_sub(value);
//}
//
// template <class Ty>
// Ty atomic_fetch_sub_explicit(volatile atomic<Ty>* target,
//                             const typename atomic<Ty>::difference_type value,
//                             const memory_order _Order) noexcept {
//  return target->fetch_sub(value, _Order);
//}
//
// template <class Ty>
// Ty atomic_fetch_sub_explicit(atomic<Ty>* target,
//                             const typename atomic<Ty>::difference_type value,
//                             const memory_order _Order) noexcept {
//  return target->fetch_sub(value, _Order);
//}
//
// template <class Ty>
// Ty atomic_fetch_and(volatile atomic<Ty>* target,
//                    const typename atomic<Ty>::value_type value) noexcept {
//  return target->fetch_and(value);
//}
//
// template <class Ty>
// Ty atomic_fetch_and(atomic<Ty>* target,
//                    const typename atomic<Ty>::value_type value) noexcept {
//  return target->fetch_and(value);
//}
//
// template <class Ty>
// Ty atomic_fetch_and_explicit(volatile atomic<Ty>* target,
//                             const typename atomic<Ty>::value_type value,
//                             const memory_order _Order) noexcept {
//  return target->fetch_and(value, _Order);
//}
//
// template <class Ty>
// Ty atomic_fetch_and_explicit(atomic<Ty>* target,
//                             const typename atomic<Ty>::value_type value,
//                             const memory_order _Order) noexcept {
//  return target->fetch_and(value, _Order);
//}
//
// template <class Ty>
// Ty atomic_fetch_or(volatile atomic<Ty>* target,
//                   const typename atomic<Ty>::value_type value) noexcept {
//  return target->fetch_or(value);
//}
//
// template <class Ty>
// Ty atomic_fetch_or(atomic<Ty>* target,
//                   const typename atomic<Ty>::value_type value) noexcept {
//  return target->fetch_or(value);
//}
//
// template <class Ty>
// Ty atomic_fetch_or_explicit(volatile atomic<Ty>* target,
//                            const typename atomic<Ty>::value_type value,
//                            const memory_order _Order) noexcept {
//  return target->fetch_or(value, _Order);
//}
//
// template <class Ty>
// Ty atomic_fetch_or_explicit(atomic<Ty>* target,
//                            const typename atomic<Ty>::value_type value,
//                            const memory_order _Order) noexcept {
//  return target->fetch_or(value, _Order);
//}
//
// template <class Ty>
// Ty atomic_fetch_xor(volatile atomic<Ty>* target,
//                    const typename atomic<Ty>::value_type value) noexcept {
//  return target->fetch_xor(value);
//}
//
// template <class Ty>
// Ty atomic_fetch_xor(atomic<Ty>* target,
//                    const typename atomic<Ty>::value_type value) noexcept {
//  return target->fetch_xor(value);
//}
//
// template <class Ty>
// Ty atomic_fetch_xor_explicit(volatile atomic<Ty>* target,
//                             const typename atomic<Ty>::value_type value,
//                             const memory_order _Order) noexcept {
//  return target->fetch_xor(value, _Order);
//}
//
// template <class Ty>
// Ty atomic_fetch_xor_explicit(atomic<Ty>* target,
//                             const typename atomic<Ty>::value_type value,
//                             const memory_order _Order) noexcept {
//  return target->fetch_xor(value, _Order);
//}
//
using atomic_bool = atomic<bool>;

using atomic_char = atomic<char>;
using atomic_schar = atomic<signed char>;
using atomic_uchar = atomic<unsigned char>;
using atomic_short = atomic<short>;
using atomic_ushort = atomic<unsigned short>;
using atomic_int = atomic<int>;
using atomic_uint = atomic<unsigned int>;
using atomic_long = atomic<long>;
using atomic_ulong = atomic<unsigned long>;
using atomic_llong = atomic<long long>;
using atomic_ullong = atomic<unsigned long long>;

using atomic_char16_t = atomic<char16_t>;
using atomic_char32_t = atomic<char32_t>;
using atomic_wchar_t = atomic<wchar_t>;

using atomic_int8_t = atomic<int8_t>;
using atomic_uint8_t = atomic<uint8_t>;
using atomic_int16_t = atomic<int16_t>;
using atomic_uint16_t = atomic<uint16_t>;
using atomic_int32_t = atomic<int32_t>;
using atomic_uint32_t = atomic<uint32_t>;
using atomic_int64_t = atomic<int64_t>;
using atomic_uint64_t = atomic<uint64_t>;

using atomic_intptr_t = atomic<intptr_t>;
using atomic_uintptr_t = atomic<uintptr_t>;
using atomic_size_t = atomic<size_t>;
using atomic_ptrdiff_t = atomic<ptrdiff_t>;
using atomic_intmax_t = atomic<intmax_t>;
using atomic_uintmax_t = atomic<uintmax_t>;
//
//// STRUCT atomic_flag
//#define ATOMIC_FLAG_INIT \
//  {}
//
// struct atomic_flag {  // flag with test-and-set semantics
//#if _HAS_CXX20
//  [[nodiscard]] bool test(
//      const memory_order _Order = memory_order_seq_cst) const noexcept {
//    return _Storage.load(_Order) != 0;
//  }
//
//  [[nodiscard]] bool test(const memory_order _Order =
//                              memory_order_seq_cst) const volatile noexcept {
//    return _Storage.load(_Order) != 0;
//  }
//#endif  // _HAS_CXX20
//
//  bool test_and_set(const memory_order _Order = memory_order_seq_cst) noexcept
//  {
//    return _Storage.exchange(true, _Order) != 0;
//  }
//
//  bool test_and_set(
//      const memory_order _Order = memory_order_seq_cst) volatile noexcept {
//    return _Storage.exchange(true, _Order) != 0;
//  }
//
//  void clear(const memory_order _Order = memory_order_seq_cst) noexcept {
//    _Storage.store(false, _Order);
//  }
//
//  void clear(
//      const memory_order _Order = memory_order_seq_cst) volatile noexcept {
//    _Storage.store(false, _Order);
//  }
//
//  constexpr atomic_flag() noexcept = default;
//
//#if 1  // TRANSITION, ABI
//  atomic<long> _Storage;
//#else   // ^^^ don't break ABI / break ABI vvv
//  atomic<bool> _Storage;
//#endif  // TRANSITION, ABI
//};
//
//// atomic_flag NONMEMBERS
//#if _HAS_CXX20
//[[nodiscard]] inline bool atomic_flag_test(
//    const volatile atomic_flag* const _Flag) noexcept {
//  return _Flag->test();
//}
//
//[[nodiscard]] inline bool atomic_flag_test(
//    const atomic_flag* const _Flag) noexcept {
//  return _Flag->test();
//}
//
//[[nodiscard]] inline bool atomic_flag_test_explicit(
//    const volatile atomic_flag* const _Flag,
//    const memory_order _Order) noexcept {
//  return _Flag->test(_Order);
//}
//
//[[nodiscard]] inline bool atomic_flag_test_explicit(
//    const atomic_flag* const _Flag,
//    const memory_order _Order) noexcept {
//  return _Flag->test(_Order);
//}
//#endif  // _HAS_CXX20
//
// inline bool atomic_flag_test_and_set(atomic_flag* const _Flag) noexcept {
//  return _Flag->test_and_set();
//}
//
// inline bool atomic_flag_test_and_set(
//    volatile atomic_flag* const _Flag) noexcept {
//  return _Flag->test_and_set();
//}
//
// inline bool atomic_flag_test_and_set_explicit(
//    atomic_flag* const _Flag,
//    const memory_order _Order) noexcept {
//  return _Flag->test_and_set(_Order);
//}
//
// inline bool atomic_flag_test_and_set_explicit(
//    volatile atomic_flag* const _Flag,
//    const memory_order _Order) noexcept {
//  return _Flag->test_and_set(_Order);
//}
//
// inline void atomic_flag_clear(atomic_flag* const _Flag) noexcept {
//  _Flag->clear();
//}
//
// inline void atomic_flag_clear(volatile atomic_flag* const _Flag) noexcept {
//  _Flag->clear();
//}
//
// inline void atomic_flag_clear_explicit(atomic_flag* const _Flag,
//                                       const memory_order _Order) noexcept {
//  _Flag->clear(_Order);
//}
//
// inline void atomic_flag_clear_explicit(volatile atomic_flag* const _Flag,
//                                       const memory_order _Order) noexcept {
//  _Flag->clear(_Order);
//}
//
//#undef ATOMIC_INTRINSIC
//#undef COMPILER_BARRIER
//
//#pragma warning(pop)
//#pragma pack(pop)
}  // namespace ktl