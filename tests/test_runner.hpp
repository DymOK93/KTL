#pragma once
#include "container_traits.hpp"

#include <runtime/include/bugcheck.hpp>
#include <runtime/include/crt_attributes.hpp>

#include <include/array.hpp>
#include <include/string.hpp>
#include <include/string_view.hpp>
#include <include/type_traits.hpp>

#include <modules/fmt/compile.hpp>

#include <ntddk.h>

namespace test::details {
struct basic_container_formatter {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) -> decltype(ktl::begin(ctx)) {
    auto first{ktl::begin(ctx)};
    const auto last{ktl::end(ctx)};
    const bool invalid_format{first != last && *first != '}'};
    ktl::throw_exception_if<ktl::format_error>(invalid_format,
                                               "invalid format");
    return first;
  }
};
}  // namespace test::details

namespace fmt {
#define EMPTY_PREFIX
#define VALUE_CONTAINER_FORMATTER(char_type, prefix)                        \
  template <class Container>                                                \
  struct formatter<Container, char_type,                                    \
                   ktl::enable_if_t<test::is_value_container_v<Container>>> \
      : test::details::basic_container_formatter {                          \
    template <class FormatContext>                                          \
    auto format(const Container& cont, FormatContext& ctx)                  \
        -> decltype(ctx.out()) {                                            \
      auto out{ktl::format_to(ctx.out(), FMT_COMPILE(prefix##"{{"))};       \
      bool first{true};                                                     \
      for (const auto& value : cont) {                                      \
        if (!first) {                                                       \
          out = ktl::format_to(out, FMT_COMPILE(prefix##", {}"), value);    \
        } else {                                                            \
          first = false;                                                    \
          out = ktl::format_to(out, FMT_COMPILE(prefix##"{}"), value);      \
        }                                                                   \
      }                                                                     \
      return ktl::format_to(out, FMT_COMPILE(prefix##"}}"));                \
    }                                                                       \
  };

#define KV_CONTAINER_FORMATTER(char_type, prefix)                             \
  template <class Container>                                                  \
  struct formatter<Container, char_type,                                      \
                   ktl::enable_if_t<test::is_kv_container_v<Container>>>      \
      : test::details::basic_container_formatter {                            \
    template <class FormatContext>                                            \
    auto format(const Container& cont, FormatContext& ctx)                    \
        -> decltype(ctx.out()) {                                              \
      auto out{ktl::format_to(ctx.out(), FMT_COMPILE(prefix##"{{"))};         \
      bool first{true};                                                       \
      for (const auto& [key, value] : cont) {                                 \
        if (!first) {                                                         \
          out = ktl::format_to(out, FMT_COMPILE(prefix##", {}: {}"), key,     \
                               value);                                        \
        } else {                                                              \
          first = false;                                                      \
          out =                                                               \
              ktl::format_to(out, FMT_COMPILE(prefix##"{}: {}"), key, value); \
        }                                                                     \
      }                                                                       \
      return ktl::format_to(out, FMT_COMPILE(prefix##"}}"));                  \
    }                                                                         \
  };

VALUE_CONTAINER_FORMATTER(char, EMPTY_PREFIX)
VALUE_CONTAINER_FORMATTER(wchar_t, L)

KV_CONTAINER_FORMATTER(char, EMPTY_PREFIX)
KV_CONTAINER_FORMATTER(wchar_t, L)

#undef KV_CONTAINER_FORMATTER
#undef VALUE_CONTAINER_FORMATTER
#undef EMPTY_PREFIX
}  // namespace fmt

namespace test {
namespace details {
void print_impl(ktl::ansi_string_view msg) noexcept;

template <class... Types>
void print(ktl::ansi_string_view format, const Types&... args) {
  const auto msg{ktl::format(format, args...)};
  print_impl(msg);
}

template <size_t N, class... Types>
void print(const char (&format)[N], const Types&... args) {
  const auto msg{ktl::format(FMT_COMPILE(format), args...)};
  print_impl(msg);
}
}  // namespace details

template <class Ty, class U>
void check_equal(const Ty& lhs, const U& rhs, ktl::ansi_string_view hint = {}) {
  if (!(lhs == rhs)) {
    auto msg{ktl::format("Assertion failed: {} != {}", lhs, rhs)};
    if (!hint.empty()) {
      msg.reserve(static_cast<typename decltype(msg)::size_type>(
          ktl::size(msg) + ktl::size(hint)));
      ktl::format_to(ktl::back_inserter(msg), ", hint: {}", hint);
    }
    const ktl::ansi_string_view msg_view{msg};
    ktl::throw_exception<ktl::runtime_error>(msg_view);
  }
}

void check(bool cond, ktl::ansi_string_view hint);

class Runner : ktl::non_relocatable {
 public:
  template <class TestFunc>
  NTSTATUS execute(TestFunc fn, ktl::ansi_string_view test_name) {
    NTSTATUS status{STATUS_SUCCESS};
    try {
      fn();
      details::print("{} OK\n", test_name);
    } catch (const ktl::exception& exc) {
      ++m_fail_count;
      details::print("{} failed: {}\n", test_name, exc.what());
      status = exc.code();
    } catch (...) {
      ++m_fail_count;
      details::print("Unknown exception caught\n");
      status = STATUS_UNHANDLED_EXCEPTION;
    }
    return status;
  }

  ~Runner() noexcept;

 private:
  ktl::crt::termination_context capture_failure_context() noexcept;

 private:
  size_t m_fail_count{0};
};
}  // namespace test

#define ASSERT_VALUE(x)                                                 \
  {                                                                     \
    const auto hint{ktl::format(FMT_COMPILE("{} is false, {}: {}"), #x, \
                                __FILE__, __LINE__)};                   \
    test::check((x), hint);                                             \
  }

#define ASSERT_EQ(x, y)                                                  \
  {                                                                      \
    const auto hint{ktl::format(FMT_COMPILE("{} != {}, {}: {}"), #x, #y, \
                                __FILE__, __LINE__)};                    \
    test::check_equal((x), (y), hint);                                   \
  }

#define RUN_TEST(tr, func) tr.execute(func, #func)
