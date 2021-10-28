#pragma once
#include "container_traits.hpp"

#include <runtime/include/bugcheck.hpp>
#include <runtime/include/crt_attributes.hpp>

#include <include/array.hpp>
#include <include/string.hpp>
#include <include/string_view.hpp>
#include <include/type_traits.hpp>
#include <include/vector.hpp>

#include <modules/fmt/compile.hpp>

#include <ntddk.h>

namespace fmt {
template <class Ty, class Alloc>
struct formatter<ktl::vector<Ty, Alloc>> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) -> decltype(ktl::begin(ctx)) {
    const auto first{ktl::begin(ctx)}, last{ktl::end(ctx)};
    const bool valid_format{first != last && first++ == '}' && first == last};
    ktl::throw_exception_if_not<ktl::format_error>(valid_format,
                                                   "invalid format");
    return first;
  }

  template <class FormatContext>
  auto format(const ktl::vector<Ty, Alloc>& vec, FormatContext& ctx)
      -> decltype(ctx.out()) {
    auto out{ktl::format_to(ctx.out(), FMT_COMPILE("{{"))};
    bool first{true};
    for (const auto& value : vec) {
      if (!first) {
        out = ktl::format_to(out, FMT_COMPILE(", {}"), value);
      } else {
        first = false;
        out = ktl::format_to(out, FMT_COMPILE("{}"), value);
      }
    }
    return ktl::format_to(out, FMT_COMPILE("}}"));
  }
};
}  // namespace fmt

namespace test {
namespace details {
// template <class OutputIt, class Ty>
// OutputIt serialize_to(OutputIt out, const Ty& value) {
//  return ktl::format_to(out, FMT_COMPILE("{}"), value);
//}
//
// template <class OutputIt,
//          class Ty,
//          ktl::enable_if_t<is_linear_v<Ty> || is_set_v<Ty>, int> = 0>
// OutputIt serialize_to(OutputIt out, const Ty& cont) {
//  out = ktl::format_to(out, FMT_COMPILE("{{"));
//  bool first = true;
//  for (const auto& key : cont) {
//    if (!first) {
//      out = ktl::format_to(out, FMT_COMPILE(", "));
//      out = serialize_to(out, key);
//    } else {
//      first = false;
//      out = serialize_to(out, key);
//    }
//  }
//  return ktl::format_to(out, FMT_COMPILE("}}"));
//}
//
// template <class OutputIt, class Ty, ktl::enable_if_t<is_map_v<Ty>, int> =
// 0> OutputIt serialize_to(OutputIt out, const Ty& map) {
//  constexpr auto kv_serializer{[](OutputIt o, const auto& kv) -> OutputIt {
//    const auto& [key, value]{kv};
//    o = serialize_to(o, key);
//    o = ktl::format_to(o, FMT_COMPILE(": "));
//    return serialize_to(o, value);
//  }};
//
//  out = ktl::format_to(out, FMT_COMPILE("{{"));
//  bool first = true;
//  for (const auto& kv : map) {
//    if (!first) {
//      out = ktl::format_to(out, FMT_COMPILE(", "));
//      out = kv_serializer(out, kv);
//    } else {
//      first = false;
//      out = kv_serializer(out, kv);
//    }
//  }
//  return ktl::format_to(out, FMT_COMPILE("}}"));
//}

void print_impl(ktl::ansi_string_view msg) noexcept;

template <class... Types>
void print(ktl::ansi_string_view, const Types&... args);

template <size_t N, class... Types>
void print(const char (&format)[N], const Types&... args);
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

#define CHECK_EQUAL(x, y)                                                \
  {                                                                      \
    const auto hint{ktl::format(FMT_COMPILE("{} != {}, {}: {}"), #x, #y, \
                                __FILE__, __LINE__)};                    \
    test::check_equal((x), (y), hint);                                   \
  }

#define CHECK(x)                                                        \
  {                                                                     \
    const auto hint{ktl::format(FMT_COMPILE("{} is false, {}: {}"), #x, \
                                __FILE__, __LINE__)};                   \
    test::check((x), hint);                                             \
  }

#define RUN_TEST(tr, func) tr.execute(func, #func)
