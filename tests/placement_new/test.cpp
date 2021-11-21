#include "test.hpp"

#include <test_runner.hpp>

#include <string.hpp>
#include <type_traits.hpp>

using namespace ktl;

namespace tests::placement_new {
static constexpr size_t REFERENCE_STR_LENGTH{21};

// Entity with trivial d-tor
struct String : non_relocatable {
  String(const char* s) noexcept { memcpy(data, s, REFERENCE_STR_LENGTH); }
  ~String() noexcept { memset(data, 0, REFERENCE_STR_LENGTH); }

  bool operator==(const char* s) const noexcept {
    return memcmp(data, s, REFERENCE_STR_LENGTH) == 0;
  }

  char data[REFERENCE_STR_LENGTH + 1]{};
};

// Volatile to prevent compiler optimizations
const char* volatile g_reference_str{nullptr};

struct StringInitializer {
  static const char REFERENCE_STR[];

  StringInitializer() noexcept { g_reference_str = REFERENCE_STR; }
};

const char StringInitializer::REFERENCE_STR[]{"C++ in Windows Kernel"};
static_assert(size(StringInitializer::REFERENCE_STR) ==
              REFERENCE_STR_LENGTH + 1);

namespace details {
StringInitializer string_initializer;
}

// With a terminating null character

void construct_on_buffer() {
  alignas(String) byte buffer[sizeof(String)];
  auto* str_in_buf{new (buffer) String{g_reference_str}};
  unique_ptr str_guard{str_in_buf, [](String* s) { s->~String(); }};
  ASSERT_EQ(str_in_buf->data, g_reference_str)
}

static bool is_zeroed(const char* buffer, size_t size) noexcept {
  while (size--) {
    if (*buffer++) {
      return false;
    }
  }
  return true;
}

void construct_after_destroying() {
  String s{g_reference_str};
  s.~String();
  ASSERT_VALUE(is_zeroed(s.data, REFERENCE_STR_LENGTH))

  auto* reconstructed_str{new (addressof(s)) String{g_reference_str}};
  ASSERT_EQ(reconstructed_str->data, g_reference_str)
}

struct ValueWithConstMember {
  const int c;  // Note: X has a const member
  int nc;
};

union TrickyUnion {
  ValueWithConstMember vwcm;
  int number;
};

void construct_with_launder() {
  TrickyUnion u{ValueWithConstMember{-1, 12}};
  u.number = 42;

  constexpr int EXPECTED_VALUE{12};
  new (addressof(u)) ValueWithConstMember{EXPECTED_VALUE, -1};
  auto* laundered_ptr{launder(addressof(u.vwcm.c))};
  ASSERT_EQ(*laundered_ptr, EXPECTED_VALUE)
}
}  // namespace tests::placement_new