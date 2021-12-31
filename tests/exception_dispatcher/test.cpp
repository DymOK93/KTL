#include "test.hpp"

#include <test_runner.hpp>

#include <array.hpp>
#include <irql.hpp>
#include <ktlexcept.hpp>
#include <mutex.hpp>

using namespace ktl;

namespace tests::exception_dispatcher {
namespace details {
static size_t g_object_count{0};
}

struct TestObject  // NOLINT(cppcoreguidelines-special-member-functions)
    : non_relocatable {
  TestObject() { ++details::g_object_count; }
  ~TestObject() noexcept { --details::g_object_count; }
};

template <size_t N, class Exc>
class ThrowAfter  // NOLINT(cppcoreguidelines-special-member-functions)
    : public non_relocatable {
 public:
  template <class... Types>
  ThrowAfter(Types&&... exc_args) {
    if (details::g_object_count >= N) {
      throw_exception<Exc>(forward<Types>(exc_args)...);
    }
    ++details::g_object_count;
  }

  ~ThrowAfter() noexcept {
    -- details::g_object_count;
  }

 private:
  TestObject m_payload;
};

struct TestException : runtime_error {
  using MyBase = runtime_error;

  TestException() : MyBase("") {}  // NOLINT(modernize-use-equals-default)

  template <class Ty,
            enable_if_t<is_constructible_v<MyBase, const Ty&>, int> = 0>
  TestException(const Ty& str) : MyBase(str) {}
};

void throw_directly() {
  using target_t = ThrowAfter<1, exception>;

  size_t instance_count{(numeric_limits<size_t>::max)()};
  try {
    [[maybe_unused]] const target_t obj1, obj2;
  } catch ([[maybe_unused]] const exception& exc) {
    instance_count = details::g_object_count;
  }
  ASSERT_EQ(instance_count, 0);
}

namespace details {
template <class Ty>
void do_call(size_t counter);

template <class Ty>
void do_call_helper(size_t counter) {
  volatile auto* do_call_ptr{&do_call<Ty>};
  if (counter > 0) {
    do_call_ptr(counter - 1);
  }
}

template <class Ty>
void do_call(size_t counter) {
  Ty obj{};
  do_call_helper<Ty>(counter);
}
}  // namespace details

void throw_in_nested_call() {
  static constexpr size_t RECURSION_DEPTH{10};
  using target_t = ThrowAfter<RECURSION_DEPTH - 1, exception>;

  size_t instance_count{(numeric_limits<size_t>::max)()};
  try {
    details::do_call<target_t>(RECURSION_DEPTH);
  } catch ([[maybe_unused]] const exception& exc) {
    instance_count = details::g_object_count;
  }
  ASSERT_EQ(instance_count, 0);
}

#define MAKE_MESSAGE(Idx) /* NOLINT(cppcoreguidelines-macro-usage)*/ \
  CONCAT(__FUNCTION__, #Idx)

void throw_on_array_init() {
  static constexpr size_t OBJECT_COUNT{5};
  using target_t = ThrowAfter<OBJECT_COUNT - 1, TestException>;

  size_t instance_count{(numeric_limits<size_t>::max)()};
  try {
    // The number of initializers is less than the number of objects
    array<target_t, OBJECT_COUNT> arr{MAKE_MESSAGE(1), MAKE_MESSAGE(2),
                                      MAKE_MESSAGE(3), MAKE_MESSAGE(4)};
  } catch ([[maybe_unused]] const exception& exc) {
    instance_count = details::g_object_count;
  }
  ASSERT_EQ(instance_count, 0);
}

#undef MAKE_MESSAGE

void throw_in_new_expression() {
  using target_t = ThrowAfter<1, exception>;

  size_t instance_count{(numeric_limits<size_t>::max)()};
  try {
    [[maybe_unused]] const target_t* obj_ptr{new target_t};
  } catch ([[maybe_unused]] const exception& exc) {
    instance_count = details::g_object_count;
  }
  ASSERT_EQ(instance_count, 0);
}

void throw_at_high_irql() {
  using target_t = ThrowAfter<1, exception>;

  const irql_t prev_irql{get_current_irql()};
  irql_t current_irql{HIGH_LEVEL};
  size_t instance_count{(numeric_limits<size_t>::max)()};
  try {
    const irql_guard irql{current_irql};
    const target_t obj;
  } catch ([[maybe_unused]] const exception& exc) {
    instance_count = details::g_object_count;
    current_irql = get_current_irql();
  }
  ASSERT_EQ(instance_count, 0);
  ASSERT_EQ(current_irql, prev_irql);
}

enum class CatchBlockType { Expected, Unexpected, Ellipsis, Unknown };

namespace details {
template <class Exc>
NTSTATUS get_code(Exc exc) {
  if constexpr (!is_pointer_v<Exc>) {
    return exc.code();
  } else {
    return exc->code();
  }
}

template <class Exc>
const char* get_what(Exc exc) {
  if constexpr (!is_pointer_v<Exc>) {
    return exc.what();
  } else {
    return exc->what();
  }
}

template <class Exc, class ExpectedTy, class... Types>
CatchBlockType catch_impl(const Types&... exc_args) {
  using target_t = ThrowAfter<1, Exc>;

  auto catch_block{CatchBlockType::Unknown};
  try {
    [[maybe_unused]] const target_t obj{exc_args...};
  } catch (ExpectedTy exc) {
    if (const auto expected = Exc{exc_args...};
        get_code<ExpectedTy>(exc) == get_code<Exc>(expected) &&
        strcmp(get_what<ExpectedTy>(exc), get_what<Exc>(expected)) == 0) {
      catch_block = CatchBlockType::Expected;
    } else {
      catch_block = CatchBlockType::Unexpected;
    }
  } catch (...) {
    catch_block = CatchBlockType::Ellipsis;
  }
  return catch_block;
}
}  // namespace details

// clang-format off
#define PREPARE_CATCH_BY /* NOLINT(cppcoreguidelines-macro-usage)*/ \
  CatchBlockType catch_block_type

#define CHECK_CATCH_BY_EX(exc, expected, ...) /* NOLINT(cppcoreguidelines-macro-usage)*/ \
  catch_block_type = (details::catch_impl<exc, expected>(__VA_ARGS__));                  \
  ASSERT_EQ(catch_block_type, CatchBlockType::Expected)

#define CHECK_CATCH_BY(expected) /* NOLINT(cppcoreguidelines-macro-usage)*/ \
  CHECK_CATCH_BY_EX(runtime_error, expected, __FUNCTION__)
// clang-format on

void catch_by_value() {
  PREPARE_CATCH_BY;
  CHECK_CATCH_BY(exception);
  CHECK_CATCH_BY(const exception);
}

void catch_by_base_ref() {
  PREPARE_CATCH_BY;
  CHECK_CATCH_BY(exception&);
  CHECK_CATCH_BY(const exception&);
}

void catch_by_base_ptr() {
  const unique_ptr exc_guard{new runtime_error{"catch_by_base_ptr"}};
  PREPARE_CATCH_BY;
  CHECK_CATCH_BY_EX(runtime_error*, exception*, exc_guard.get());
  CHECK_CATCH_BY_EX(runtime_error*, const exception*, exc_guard.get());
}
}  // namespace tests::exception_dispatcher
