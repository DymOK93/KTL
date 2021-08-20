#pragma once
#include <basic_types.h>
#include <chrono_impl.h>
#include <limits.hpp>
#include <ratio.hpp>
#include <type_traits.hpp>

#include <ntddk.h>

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
inline constexpr bool is_duration_v = is_specialization_v<Ty, duration>;

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
                "duration must be an instance of ktl::duration");

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

template <intmax_t Lhs, intmax_t rhs>
struct lcm : integral_constant<intmax_t, (Lhs / gcd<Lhs, rhs>::value) * rhs> {
};  // compute least common multiple of Lhs and rhs

template <class LhsRep, class LhsPeriod, class RhsRep, class RhsPeriod>
struct common_type<chrono::duration<LhsRep, LhsPeriod>,
                   chrono::duration<RhsRep, RhsPeriod>> {
  using type =
      chrono::duration<common_type_t<LhsRep, RhsRep>,
                       ratio<gcd<LhsPeriod::num, RhsPeriod::num>::value,
                             lcm<LhsPeriod::den, RhsPeriod::den>::value>>;
};

template <class Clock, class LhsDuration, class RhsDuration>
struct common_type<chrono::time_point<Clock, LhsDuration>,
                   chrono::time_point<Clock, RhsDuration>> {
  using type =
      chrono::time_point<Clock, common_type_t<LhsDuration, RhsDuration>>;
};

namespace chrono {
template <class LhsRep, class LhsPeriod, class RhsRep, class RhsPeriod>
[[nodiscard]] constexpr common_type_t<duration<LhsRep, LhsPeriod>,
                                      duration<RhsRep, RhsPeriod>>
operator+(const duration<LhsRep, LhsPeriod>& lhs,
          const duration<RhsRep, RhsPeriod>&
              rhs) noexcept(is_arithmetic_v<LhsRep>&& is_arithmetic_v<RhsRep>) {
  using common_duration =
      common_type_t<duration<LhsRep, LhsPeriod>, duration<RhsRep, RhsPeriod>>;
  return common_duration(common_duration(lhs).count() +
                         common_duration(rhs).count());
}

template <class LhsRep, class LhsPeriod, class RhsRep, class RhsPeriod>
[[nodiscard]] constexpr common_type_t<duration<LhsRep, LhsPeriod>,
                                      duration<RhsRep, RhsPeriod>>
operator-(const duration<LhsRep, LhsPeriod>& lhs,
          const duration<RhsRep, RhsPeriod>&
              rhs) noexcept(is_arithmetic_v<LhsRep>&& is_arithmetic_v<RhsRep>) {
  using common_duration =
      common_type_t<duration<LhsRep, LhsPeriod>, duration<RhsRep, RhsPeriod>>;
  return common_duration(common_duration(lhs).count() -
                         common_duration(rhs).count());
}

template <
    class LhsRep,
    class LhsPeriod,
    class RhsRep,
    enable_if_t<is_convertible_v<const RhsRep&, common_type_t<LhsRep, RhsRep>>,
                int> = 0>
[[nodiscard]] constexpr duration<common_type_t<LhsRep, RhsRep>, LhsPeriod>
operator*(const duration<LhsRep, LhsPeriod>& lhs, const RhsRep& rhs) noexcept(
    is_arithmetic_v<LhsRep>&& is_arithmetic_v<RhsRep>) {
  using common_rep = common_type_t<LhsRep, RhsRep>;
  using common_duration = duration<common_rep, LhsPeriod>;
  return common_duration(common_duration(lhs).count() * rhs);
}

template <
    class LhsRep,
    class RhsRep,
    class RhsPeriod,
    enable_if_t<is_convertible_v<const LhsRep&, common_type_t<LhsRep, RhsRep>>,
                int> = 0>
[[nodiscard]] constexpr duration<common_type_t<LhsRep, RhsRep>, RhsPeriod>
operator*(const LhsRep& lhs, const duration<RhsRep, RhsPeriod>& rhs) noexcept(
    is_arithmetic_v<LhsRep>&& is_arithmetic_v<RhsRep>) {
  return rhs * lhs;
}

template <class CommonRep,
          class LhsPeriod,
          class RhsRep,
          bool = is_convertible_v<const RhsRep&, CommonRep>>
struct duration_div_mod1 {  // return type for duration / rep and duration %
                            // rep
  using type = duration<CommonRep, LhsPeriod>;
};

template <class CommonRep, class LhsPeriod, class RhsRep>
struct duration_div_mod1<CommonRep, LhsPeriod, RhsRep, false> {
};  // no return type

template <class CommonRep,
          class LhsPeriod,
          class RhsRep,
          bool = is_duration_v<RhsRep>>
struct duration_div_mod {};  // no return type

template <class CommonRep, class LhsPeriod, class RhsRep>
struct duration_div_mod<CommonRep, LhsPeriod, RhsRep, false>
    : duration_div_mod1<CommonRep, LhsPeriod, RhsRep> {
  // return type for duration / rep and duration % rep
};

template <class LhsRep, class LhsPeriod, class RhsRep>
[[nodiscard]] constexpr typename duration_div_mod<common_type_t<LhsRep, RhsRep>,
                                                  LhsPeriod,
                                                  RhsRep>::type
operator/(const duration<LhsRep, LhsPeriod>& lhs, const RhsRep& rhs) noexcept(
    is_arithmetic_v<LhsRep>&& is_arithmetic_v<RhsRep>) {
  using common_rep = common_type_t<LhsRep, RhsRep>;
  using common_duration = duration<common_rep, LhsPeriod>;
  return common_duration(common_duration(lhs).count() / rhs);
}

template <class LhsRep, class LhsPeriod, class RhsRep, class RhsPeriod>
[[nodiscard]] constexpr common_type_t<LhsRep, RhsRep> operator/(
    const duration<LhsRep, LhsPeriod>& lhs,
    const duration<RhsRep, RhsPeriod>&
        rhs) noexcept(is_arithmetic_v<LhsRep>&& is_arithmetic_v<RhsRep>) {
  using common_duration =
      common_type_t<duration<LhsRep, LhsPeriod>, duration<RhsRep, RhsPeriod>>;
  return common_duration(lhs).count() / common_duration(rhs).count();
}

template <class LhsRep, class LhsPeriod, class RhsRep>
[[nodiscard]] constexpr typename duration_div_mod<common_type_t<LhsRep, RhsRep>,
                                                  LhsPeriod,
                                                  RhsRep>::type
operator%(const duration<LhsRep, LhsPeriod>& lhs, const RhsRep& rhs) noexcept(
    is_arithmetic_v<LhsRep>&& is_arithmetic_v<RhsRep>) {
  using common_rep = common_type_t<LhsRep, RhsRep>;
  using common_duration = duration<common_rep, LhsPeriod>;
  return common_duration(common_duration(lhs).count() % rhs);
}

template <class LhsRep, class LhsPeriod, class RhsRep, class RhsPeriod>
[[nodiscard]] constexpr common_type_t<duration<LhsRep, LhsPeriod>,
                                      duration<RhsRep, RhsPeriod>>
operator%(const duration<LhsRep, LhsPeriod>& lhs,
          const duration<RhsRep, RhsPeriod>&
              rhs) noexcept(is_arithmetic_v<LhsRep>&& is_arithmetic_v<RhsRep>) {
  using common_duration =
      common_type_t<duration<LhsRep, LhsPeriod>, duration<RhsRep, RhsPeriod>>;
  return common_duration(common_duration(lhs).count() %
                         common_duration(rhs).count());
}

template <class LhsRep, class LhsPeriod, class RhsRep, class RhsPeriod>
[[nodiscard]] constexpr bool operator==(
    const duration<LhsRep, LhsPeriod>& lhs,
    const duration<RhsRep, RhsPeriod>&
        rhs) noexcept(is_arithmetic_v<LhsRep>&& is_arithmetic_v<RhsRep>) {
  using common_t =
      common_type_t<duration<LhsRep, LhsPeriod>, duration<RhsRep, RhsPeriod>>;
  return common_t(lhs).count() == common_t(rhs).count();
}

template <class LhsRep, class LhsPeriod, class RhsRep, class RhsPeriod>
[[nodiscard]] constexpr bool operator!=(
    const duration<LhsRep, LhsPeriod>& lhs,
    const duration<RhsRep, RhsPeriod>&
        rhs) noexcept(is_arithmetic_v<LhsRep>&& is_arithmetic_v<RhsRep>) {
  return !(lhs == rhs);
}

template <class LhsRep, class LhsPeriod, class RhsRep, class RhsPeriod>
[[nodiscard]] constexpr bool operator<(
    const duration<LhsRep, LhsPeriod>& lhs,
    const duration<RhsRep, RhsPeriod>&
        rhs) noexcept(is_arithmetic_v<LhsRep>&& is_arithmetic_v<RhsRep>) {
  using common_t =
      common_type_t<duration<LhsRep, LhsPeriod>, duration<RhsRep, RhsPeriod>>;
  return common_t(lhs).count() < common_t(rhs).count();
}

template <class LhsRep, class LhsPeriod, class RhsRep, class RhsPeriod>
[[nodiscard]] constexpr bool operator<=(
    const duration<LhsRep, LhsPeriod>& lhs,
    const duration<RhsRep, RhsPeriod>&
        rhs) noexcept(is_arithmetic_v<LhsRep>&& is_arithmetic_v<RhsRep>) {
  return !(rhs < lhs);
}

template <class LhsRep, class LhsPeriod, class RhsRep, class RhsPeriod>
[[nodiscard]] constexpr bool operator>(
    const duration<LhsRep, LhsPeriod>& lhs,
    const duration<RhsRep, RhsPeriod>&
        rhs) noexcept(is_arithmetic_v<LhsRep>&& is_arithmetic_v<RhsRep>) {
  return rhs < lhs;
}

template <class LhsRep, class LhsPeriod, class RhsRep, class RhsPeriod>
[[nodiscard]] constexpr bool operator>=(
    const duration<LhsRep, LhsPeriod>& lhs,
    const duration<RhsRep, RhsPeriod>&
        rhs) noexcept(is_arithmetic_v<LhsRep>&& is_arithmetic_v<RhsRep>) {
  return !(lhs < rhs);
}

template <class To,
          class Rep,
          class Period,
          enable_if_t<is_duration_v<To>, int> Enabled>
[[nodiscard]] constexpr To
duration_cast(const duration<Rep, Period>& dur) noexcept(
    is_arithmetic_v<Rep>&& is_arithmetic_v<typename To::rep>) {
  using carry_flag = ratio_divide<Period, typename To::period>;

  using ToRep = typename To::rep;
  using common_rep = common_type_t<ToRep, Rep, intmax_t>;

  constexpr bool num_is_one = carry_flag::num == 1;
  constexpr bool den_is_one = carry_flag::den == 1;

  if constexpr (den_is_one) {
    if constexpr (num_is_one) {
      return static_cast<To>(static_cast<ToRep>(dur.count()));
    } else {
      return static_cast<To>(
          static_cast<ToRep>(static_cast<common_rep>(dur.count()) *
                             static_cast<common_rep>(carry_flag::num)));
    }
  } else {
    if constexpr (num_is_one) {
      return static_cast<To>(
          static_cast<ToRep>(static_cast<common_rep>(dur.count()) /
                             static_cast<common_rep>(carry_flag::den)));
    } else {
      return static_cast<To>(
          static_cast<ToRep>(static_cast<common_rep>(dur.count()) *
                             static_cast<common_rep>(carry_flag::num) /
                             static_cast<common_rep>(carry_flag::den)));
    }
  }
}

template <class To,
          class Rep,
          class Period,
          enable_if_t<is_duration_v<To>, int> = 0>
[[nodiscard]] constexpr To floor(const duration<Rep, Period>& dur) noexcept(
    is_arithmetic_v<Rep>&& is_arithmetic_v<typename To::rep>) {
  // convert duration to another duration; round towards negative infinity
  // i.e. the greatest integral result such that the result <= dur
  const To casted{chrono::duration_cast<To>(dur)};
  if (casted > dur) {
    return To{casted.count() - static_cast<typename To::rep>(1)};
  }

  return casted;
}

template <class To,
          class Rep,
          class Period,
          enable_if_t<is_duration_v<To>, int> = 0>
[[nodiscard]] constexpr To ceil(const duration<Rep, Period>& dur) noexcept(
    is_arithmetic_v<Rep>&& is_arithmetic_v<typename To::rep>) {
  // convert duration to another duration; round towards positive infinity
  // i.e. the least integral result such that dur <= the result
  const To casted{chrono::duration_cast<To>(dur)};
  if (casted < dur) {
    return To{casted.count() + static_cast<typename To::rep>(1)};
  }

  return casted;
}

// FUNCTION TEMPLATE round
template <class Rep>
constexpr bool is_even(Rep value) noexcept(is_arithmetic_v<Rep>) {
  // Tests whether value is even
  return value % 2 == 0;
}

template <class To,
          class Rep,
          class Period,
          enable_if_t<is_duration_v<To> &&
                          !treat_as_floating_point_v<typename To::rep>,
                      int> = 0>
[[nodiscard]] constexpr To round(const duration<Rep, Period>& dur) noexcept(
    is_arithmetic_v<Rep>&& is_arithmetic_v<typename To::rep>) {
  // convert duration to another duration, round to nearest, ties to even
  const To floored{chrono::floor<To>(dur)};
  const To ceiled{floored + To{1}};
  const auto floor_adjustment = dur - floored;
  const auto ceil_adjustment = ceiled - dur;
  if (floor_adjustment < ceil_adjustment ||
      (floor_adjustment == ceil_adjustment && is_even(floored.count()))) {
    return floored;
  }
  return ceiled;
}

template <class Rep,
          class Period,
          enable_if_t<numeric_limits<Rep>::is_signed, int> = 0>
[[nodiscard]] constexpr duration<Rep, Period> abs(
    const duration<Rep, Period> dur) noexcept(is_arithmetic_v<Rep>) {
  // create a duration with count() the absolute value of dur.count()
  return dur < duration<Rep, Period>::zero()
             ? duration<Rep, Period>::zero() - dur
             : dur;
}

using nanoseconds  = duration<long long, nano>;
using microseconds = duration<long long, micro>;
using milliseconds = duration<long long, milli>;
using seconds      = duration<long long>;
using minutes      = duration<int, ratio<60>>;
using hours        = duration<int, ratio<3600>>;
using days         = duration<int, ratio<86400>>;
using weeks        = duration<int, ratio<604800>>;
using months       = duration<int, ratio<2629746>>;
using years        = duration<int, ratio<31556952>>;
    
// native Windows 100ns duration
using tics         = duration<long long, ratio<1, 10'000'000>>;

template <class Clock, class Duration, class Rep, class Period>
[[nodiscard]] constexpr time_point<
    Clock,
    common_type_t<Duration, duration<Rep, Period>>>
operator+(const time_point<Clock, Duration>& lhs,
          const duration<Rep, Period>&
              rhs) noexcept(is_arithmetic_v<typename Duration::rep>&&
                                is_arithmetic_v<Rep>) {
  using result_t =
      time_point<Clock, common_type_t<Duration, duration<Rep, Period>>>;
  return result_t{lhs.time_since_epoch() + rhs};
}

template <class Rep, class Period, class Clock, class Duration>
[[nodiscard]] constexpr time_point<
    Clock,
    common_type_t<duration<Rep, Period>, Duration>>
operator+(const duration<Rep, Period>& lhs,
          const time_point<Clock, Duration>&
              rhs) noexcept(is_arithmetic_v<Rep>&&
                                is_arithmetic_v<typename Duration::rep>) {
  return rhs + lhs;
}

template <class Clock, class Duration, class Rep, class Period>
[[nodiscard]] constexpr time_point<
    Clock,
    common_type_t<Duration, duration<Rep, Period>>>
operator-(const time_point<Clock, Duration>& lhs,
          const duration<Rep, Period>&
              rhs) noexcept(is_arithmetic_v<typename duration::rep>&&
                                is_arithmetic_v<Rep>) {
  using result_t =
      time_point<Clock, common_type_t<duration, duration<Rep, Period>>>;
  return result_t{lhs.time_since_epoch() - rhs};
}

template <class Clock, class LhsDuration, class RhsDuration>
[[nodiscard]] constexpr common_type_t<LhsDuration, RhsDuration> operator-(
    const time_point<Clock, LhsDuration>& lhs,
    const time_point<Clock, RhsDuration>&
        rhs) noexcept(is_arithmetic_v<typename LhsDuration::rep>&&
                          is_arithmetic_v<typename RhsDuration::rep>) {
  return lhs.time_since_epoch() - rhs.time_since_epoch();
}

template <class Clock, class LhsDuration, class RhsDuration>
[[nodiscard]] constexpr bool operator==(
    const time_point<Clock, LhsDuration>& lhs,
    const time_point<Clock, RhsDuration>&
        rhs) noexcept(is_arithmetic_v<typename LhsDuration::rep>&&
                          is_arithmetic_v<typename RhsDuration::rep>) {
  return lhs.time_since_epoch() == rhs.time_since_epoch();
}

template <class Clock, class LhsDuration, class RhsDuration>
[[nodiscard]] constexpr bool operator!=(
    const time_point<Clock, LhsDuration>& lhs,
    const time_point<Clock, RhsDuration>&
        rhs) noexcept(is_arithmetic_v<typename LhsDuration::rep>&&
                          is_arithmetic_v<typename RhsDuration::rep>) {
  return !(lhs == rhs);
}

template <class Clock, class LhsDuration, class RhsDuration>
[[nodiscard]] constexpr bool operator<(
    const time_point<Clock, LhsDuration>& lhs,
    const time_point<Clock, RhsDuration>&
        rhs) noexcept(is_arithmetic_v<typename LhsDuration::rep>&&
                          is_arithmetic_v<typename RhsDuration::rep>) {
  return lhs.time_since_epoch() < rhs.time_since_epoch();
}

template <class Clock, class LhsDuration, class RhsDuration>
[[nodiscard]] constexpr bool operator<=(
    const time_point<Clock, LhsDuration>& lhs,
    const time_point<Clock, RhsDuration>&
        rhs) noexcept(is_arithmetic_v<typename LhsDuration::rep>&&
                          is_arithmetic_v<typename RhsDuration::rep>) {
  return !(rhs < lhs);
}

template <class Clock, class LhsDuration, class RhsDuration>
[[nodiscard]] constexpr bool operator>(
    const time_point<Clock, LhsDuration>& lhs,
    const time_point<Clock, RhsDuration>&
        rhs) noexcept(is_arithmetic_v<typename LhsDuration::rep>&&
                          is_arithmetic_v<typename RhsDuration::rep>) {
  return rhs < lhs;
}

template <class Clock, class LhsDuration, class RhsDuration>
[[nodiscard]] constexpr bool operator>=(
    const time_point<Clock, LhsDuration>& lhs,
    const time_point<Clock, RhsDuration>&
        rhs) noexcept(is_arithmetic_v<typename LhsDuration::rep>&&
                          is_arithmetic_v<typename RhsDuration::rep>) {
  return !(lhs < rhs);
}

template <class To,
          class Clock,
          class Duration,
          enable_if_t<is_duration_v<To>, int> = 0>
[[nodiscard]] constexpr time_point<Clock, To>
time_point_cast(const time_point<Clock, Duration>& time) noexcept(
    is_arithmetic_v<typename duration::rep>&&
        is_arithmetic_v<typename To::rep>) {
  // change the duration type of a time_point; truncate
  return time_point<Clock, To>(
      chrono::duration_cast<To>(time.time_since_epoch()));
}

template <class To,
          class Clock,
          class Duration,
          enable_if_t<is_duration_v<To>, int> = 0>
[[nodiscard]] constexpr time_point<Clock, To>
floor(const time_point<Clock, Duration>& time) noexcept(
    is_arithmetic_v<typename Duration::rep>&&
        is_arithmetic_v<typename To::rep>) {
  // change the duration type of a time_point; round towards negative infinity
  return time_point<Clock, To>(chrono::floor<To>(time.time_since_epoch()));
}

template <class To,
          class Clock,
          class Duration,
          enable_if_t<is_duration_v<To>, int> = 0>
[[nodiscard]] constexpr time_point<Clock, To>
ceil(const time_point<Clock, Duration>& time) noexcept(
    is_arithmetic_v<typename Duration::rep>&&
        is_arithmetic_v<typename To::rep>) {
  // change the duration type of a time_point; round towards positive infinity
  return time_point<Clock, To>(chrono::ceil<To>(time.time_since_epoch()));
}

template <class To,
          class Clock,
          class duration,
          enable_if_t<is_duration_v<To> &&
                          !treat_as_floating_point_v<typename To::rep>,
                      int> = 0>
[[nodiscard]] constexpr time_point<Clock, To>
round(const time_point<Clock, duration>& time) noexcept(
    is_arithmetic_v<typename duration::rep>&&
        is_arithmetic_v<typename To::rep>) {
  // change the duration type of a time_point; round to nearest, ties to even
  return time_point<Clock, To>(chrono::round<To>(time.time_since_epoch()));
}

struct system_clock {  // KeQuerySystemTimePrecise (Win8+)/KeQuerySystemTime
  using rep = long long;
  using period = ratio<1, rat::details::pow10(8)>;  // 100 nanoseconds
  using duration = chrono::duration<rep, period>;
  using time_point = chrono::time_point<system_clock>;
  static constexpr bool is_steady = false;

  [[nodiscard]] static time_point now() noexcept {  // get current time
    return time_point{duration{query_system_time()}};
  }

  [[nodiscard]] static __time64_t to_time_t(
      const time_point& time) noexcept {  // convert to __time64_t
    return duration_cast<seconds>(time.time_since_epoch()).count();
  }

  [[nodiscard]] static time_point from_time_t(
      __time64_t tm) noexcept {  // convert from __time64_t
    return time_point{seconds{tm}};
  }
};

struct steady_clock {  // KeQueryPerformanceCounter
  using rep = long long;
  using period = nano;
  using duration = nanoseconds;
  using time_point = chrono::time_point<steady_clock>;
  static constexpr bool is_steady = true;

  [[nodiscard]] static time_point now() noexcept {  // get current time
    const auto freq{
        query_performance_counter_frequency()};  // cached at driver loading
    const auto counter{query_performance_counter()};
    static_assert(period::num == 1, "This assumes period::num == 1");
    // algorithm below prevents overflow when counter is sufficiently
    // large
    const auto whole{(counter / freq) * period::den};
    const long long part{(counter % freq) * period::den / freq};
    return time_point{duration{whole + part}};
  }
};

using high_resolution_clock = steady_clock;

#define IF_PERIOD_RETURN_SUFFIX_ELSE(type, suffix) \
  if constexpr (is_same_v<Period, type>) {         \
    if constexpr (is_same_v<CharT, char>) {        \
      return suffix;                               \
    } else {                                       \
      return L##suffix;                            \
    }                                              \
  } else

template <class CharT, class Period>
[[nodiscard]] constexpr const CharT* get_literal_unit_suffix() {
  IF_PERIOD_RETURN_SUFFIX_ELSE(atto, "as")
  IF_PERIOD_RETURN_SUFFIX_ELSE(femto, "fs")
  IF_PERIOD_RETURN_SUFFIX_ELSE(pico, "ps")
  IF_PERIOD_RETURN_SUFFIX_ELSE(nano, "ns")
  IF_PERIOD_RETURN_SUFFIX_ELSE(micro, "us")
  IF_PERIOD_RETURN_SUFFIX_ELSE(milli, "ms")
  IF_PERIOD_RETURN_SUFFIX_ELSE(centi, "cs")
  IF_PERIOD_RETURN_SUFFIX_ELSE(deci, "ds")
  IF_PERIOD_RETURN_SUFFIX_ELSE(seconds::period, "s")
  IF_PERIOD_RETURN_SUFFIX_ELSE(deca, "das")
  IF_PERIOD_RETURN_SUFFIX_ELSE(hecto, "hs")
  IF_PERIOD_RETURN_SUFFIX_ELSE(kilo, "ks")
  IF_PERIOD_RETURN_SUFFIX_ELSE(mega, "Ms")
  IF_PERIOD_RETURN_SUFFIX_ELSE(giga, "Gs")
  IF_PERIOD_RETURN_SUFFIX_ELSE(tera, "Ts")
  IF_PERIOD_RETURN_SUFFIX_ELSE(peta, "Ps")
  IF_PERIOD_RETURN_SUFFIX_ELSE(exa, "Es")
  IF_PERIOD_RETURN_SUFFIX_ELSE(minutes::period, "min")
  IF_PERIOD_RETURN_SUFFIX_ELSE(hours::period, "h")
  IF_PERIOD_RETURN_SUFFIX_ELSE(ratio<86400>, "d")

  {
    return nullptr;
  }
}
#undef _IF_PERIOD_RETURN_SUFFIX_ELSE
}  // namespace chrono

inline namespace literals {
inline namespace chrono_literals {
[[nodiscard]] constexpr chrono::hours operator"" _h(
    unsigned long long value) noexcept {
  return chrono::hours{value};
}

[[nodiscard]] constexpr chrono::duration<double, ratio<3600>> operator"" _h(
    long double value) noexcept {
  return chrono::duration<double, ratio<3600>>{value};
}

[[nodiscard]] constexpr chrono::minutes(operator"" _min)(
    unsigned long long value) noexcept {
  return chrono::minutes{value};
}

[[nodiscard]] constexpr chrono::duration<double, ratio<60>>(operator"" _min)(
    long double value) noexcept {
  return chrono::duration<double, ratio<60>>{value};
}

[[nodiscard]] constexpr chrono::seconds operator"" _s(
    unsigned long long value) noexcept {
  return chrono::seconds{value};
}

[[nodiscard]] constexpr chrono::duration<double> operator"" _s(
    long double value) noexcept {
  return chrono::duration<double>{value};
}

[[nodiscard]] constexpr chrono::milliseconds operator"" _ms(
    unsigned long long value) noexcept {
  return chrono::milliseconds{value};
}

[[nodiscard]] constexpr chrono::duration<double, milli> operator"" _ms(
    long double value) noexcept {
  return chrono::duration<double, milli>{value};
}

[[nodiscard]] constexpr chrono::microseconds operator"" _us(
    unsigned long long value) noexcept {
  return chrono::microseconds{value};
}

[[nodiscard]] constexpr chrono::duration<double, micro> operator"" _us(
    long double value) noexcept {
  return chrono::duration<double, micro>{value};
}

[[nodiscard]] constexpr chrono::nanoseconds operator"" _ns(
    unsigned long long value) noexcept {
  return chrono::nanoseconds{value};
}

[[nodiscard]] constexpr chrono::duration<double, nano> operator"" _ns(
    long double value) noexcept {
  return chrono::duration<double, nano>{value};
}
}  // namespace chrono_literals
}  // namespace literals

namespace chrono {
using namespace literals::chrono_literals;

using duration_t =
    uint32_t;  // Very old crutch for the sync primitives; will be removed soon

inline LARGE_INTEGER to_native_100ns_tics(duration_t period) noexcept {
  LARGE_INTEGER native_period;
  native_period.QuadPart = period;
  return native_period;
}

template<class Rep, class Period>
auto to_native_100ns_duration(const chrono::duration<Rep, Period>& duration) {
  using native_duration_t = chrono::duration<long long, ratio<1, 10'000'000>>;
  return chrono::duration_cast<native_duration_t>(duration);
}

}  // namespace chrono
}  // namespace ktl
