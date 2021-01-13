#pragma once
#include <basic_types.h>
#include <ntddk.h>

namespace ktl::crt {
inline constexpr auto KTL_EXCEPTION{
    static_cast<NTSTATUS>(0xEA9B0000)};  // 0xE, ('K' - 'A') % 10, ('T' - 'A') %
                                         // 10, ('L' - 'A') % 10
inline constexpr auto KTL_CRT_FAILURE{
    static_cast<NTSTATUS>(KTL_EXCEPTION | 0x0000FFFF)};

struct ExceptionBase {
  virtual ~ExceptionBase() = default;
};

bool is_ktl_exception(NTSTATUS exc_code);

void init_exception_environment();
ExceptionBase* get_exception(NTSTATUS exc_code);
void replace_exception(NTSTATUS old_exc_code, ExceptionBase* new_exc);
void remove_exception(NTSTATUS exc_code);
void cleanup_exception_environment();

[[noreturn]] void throw_exception(ExceptionBase* exc_ptr);
[[noreturn]] void throw_again(NTSTATUS exc_code);
[[noreturn]] void throw_ktl_crt_failure();
}  // namespace ktl::crt