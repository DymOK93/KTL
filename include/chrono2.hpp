#pragma once
#include <limits.hpp>
#include <ratio.hpp>
#include <type_traits.hpp>

namespace ktl {
namespace chrono {
template <class Rep>  // 'Rep' is 'Representation'
struct treat_as_floating_point : is_floating_point<Rep> {
};  // tests for floating-point type

template <class Rep>
inline constexpr bool treat_as_floating_point_v =
    treat_as_floating_point<Rep>::value;

template <class Rep>
struct duration_values {
  [[nodiscard]] static constexpr Rep zero() noexcept { return Rep{0}; }

  [[nodiscard]] static constexpr Rep(min)() noexcept {
    return numeric_limits<Rep>::lowest();
  }

  [[nodiscard]] static constexpr Rep(max)() noexcept {
    return (numeric_limits<Rep>::max)();
  }
};

template <class Rep, class period = ratio<1>>
class duration;

template <class Ty>
inline constexpr bool is_duration_v = _Is_specialization_v<Ty, duration>;

template <class To,
          class Rep,
          class period,
          enable_if_t<is_duration_v<To>, int> = 0>
constexpr To duration_cast(const duration<Rep, period>&) noexcept(
    is_arithmetic_v<Rep>&& is_arithmetic_v<typename To::rep>); 

template <class Rep, class Period>
class duration {  // represents a time duration
 public:
  using rep = Rep;
  using period = typename Period::type;

  static_assert(!is_duration_v<Rep>,
                "duration can't have duration as first template argument");
  static_assert(is_ratio_v<Period>, "period not an instance of std::ratio");
  static_assert(0 < Period::num, "period negative or zero");

 public:
  constexpr duration() = default;

  template <class OtherRep,
            enable_if_t<is_convertible_v<const OtherRep&, Rep> &&
                            (treat_as_floating_point_v<Rep> ||
                             !treat_as_floating_point_v<OtherRep>),
                        int> = 0>
  constexpr explicit duration(const OtherRep& value) noexcept(
      is_arithmetic_v<Rep>&& is_arithmetic_v<OtherRep>)  // strengthened
      : m_rep(static_cast<Rep>(value)) {}

  template <
      class OtherRep,
      class OtherPeriod,
      enable_if_t<treat_as_floating_point_v<Rep> ||
                      (ratio_divide_sfinae<OtherPeriod, period>::den == 1 &&
                       !treat_as_floating_point_v<OtherRep>),
                  int> = 0>
  constexpr duration(const duration<OtherRep, OtherPeriod>& other) noexcept(
      is_arithmetic_v<Rep>&& is_arithmetic_v<OtherRep>)
      : m_rep{chrono::duration_cast<duration>(other).count()} {}

  [[nodiscard]] constexpr Rep count() const noexcept(is_arithmetic_v<Rep>) {
    return m_rep;
  }

  [[nodiscard]] constexpr common_type_t<duration> operator+() const
      noexcept(is_arithmetic_v<Rep>) {
    return common_type_t<duration>{*this};
  }

  [[nodiscard]] constexpr common_type_t<duration> operator-() const
      noexcept(is_arithmetic_v<Rep>) {
    return common_type_t<duration>{-m_rep};
  }

  constexpr duration& operator++() noexcept(is_arithmetic_v<Rep>) {
    ++m_rep;
    return *this;
  }

  constexpr duration operator++(int) noexcept(is_arithmetic_v<Rep>) {
    return duration{m_rep++};
  }

  constexpr duration& operator--() noexcept(is_arithmetic_v<Rep>) {
    --m_rep;
    return *this;
  }

  constexpr duration operator--(int) noexcept(is_arithmetic_v<Rep>) {
    return duration(m_rep--);
  }

  constexpr duration& operator+=(const duration& other) noexcept(
      is_arithmetic_v<Rep>) {
    m_rep += other.m_rep;
    return *this;
  }

  constexpr duration& operator-=(const duration& other) noexcept(
      is_arithmetic_v<Rep>) {
    m_rep -= other.m_rep;
    return *this;
  }

  constexpr duration& operator*=(const Rep& other) noexcept(
      is_arithmetic_v<Rep>) {
    m_rep *= other;
    return *this;
  }

  constexpr duration& operator/=(const Rep& other) noexcept(
      is_arithmetic_v<Rep>) {
    m_rep /= other;
    return *this;
  }

  constexpr duration& operator%=(const Rep& other) noexcept(
      is_arithmetic_v<Rep>) {
    m_rep %= other;
    return *this;
  }

  constexpr duration& operator%=(const duration& other) noexcept(
      is_arithmetic_v<Rep>) {
    m_rep %= other.count();
    return *this;
  }

  [[nodiscard]] static constexpr duration zero() noexcept {
    return duration{duration_values<Rep>::zero()};
  }

  [[nodiscard]] static constexpr duration(min)() noexcept {
    return duration{(duration_values<Rep>::min)()};
  }

  [[nodiscard]] static constexpr duration(max)() noexcept {
    return duration{(duration_values<Rep>::max)()};
  }

 private:
  Rep m_rep; 
};

template <class ClockTy, class Duration = typename ClockTy::duration>
class time_point {
 public:
  using clock = ClockTy;
  using duration = Duration;
  using rep = typename Duration::rep;
  using period = typename Duration::period;

  static_assert(is_duration_v<Duration>,
                "duration must be an instance of std::duration");

  constexpr time_point() = default;

  constexpr explicit time_point(const Duration& other) noexcept(
      is_arithmetic_v<rep>)  // strengthened
      : m_duration(other) {}

  template <class OtherDuration,
            enable_if_t<is_convertible_v<OtherDuration, Duration>, int> = 0>
  constexpr time_point(
      const time_point<ClockTy, OtherDuration>&
          other) noexcept(is_arithmetic_v<rep>&&
                              is_arithmetic_v<
                                  typename OtherDuration::rep>)  // strengthened
      : m_duration{other.time_since_epoch()} {}

  [[nodiscard]] constexpr Duration time_since_epoch() const
      noexcept(is_arithmetic_v<rep>) {
    return m_duration;
  }

  constexpr time_point& operator+=(const Duration& other) noexcept(
      is_arithmetic_v<rep>) {
    m_duration += other;
    return *this;
  }

  constexpr time_point& operator-=(const Duration& other) noexcept(
      is_arithmetic_v<rep>) {
    m_duration -= other;
    return *this;
  }

  [[nodiscard]] static constexpr time_point(min)() noexcept {
    return time_point{(Duration::min)()};
  }

  [[nodiscard]] static constexpr time_point(max)() noexcept {
    return time_point{(Duration::max)()};
  }

 private:
  Duration m_duration{duration::zero()};  // duration since the epoch
};
}  // namespace chrono
}  // namespace ktl