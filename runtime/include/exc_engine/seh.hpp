#pragma once
#include <../crt_attributes.h>
#include <flag_set.hpp>

namespace ktl::crt::exc_engine::win {
enum class ExceptionFlag : uint32_t {
  NonContinuable = 0x01,
  Unwinding = 0x02,
  ExitUnwind = 0x04,
  StackInvalid = 0x08,
  NestedCall = 0x10,
  TargetUnwind = 0x20,
  CollidedUnwind = 0x40,
};

struct exception_record {
  uint32_t code;
  flag_set<ExceptionFlag> flags;
  exception_record* next;
  const byte* address;
  uint32_t parameter_count;
};

enum class ExceptionDisposition {
  ContinueExecution = 0,
  ContinueSearch = 1,
  Nested = 2,
  Collided = 3,
  CxxHandler = 0x154d3c64,
};

struct x86_cpu_context;
struct x64_cpu_context;

struct x86_seh_registration;

using x86_frame_handler_t =
    ExceptionDisposition CRTCALL(exception_record* exception_record,
                                 x86_seh_registration* registration,
                                 x86_cpu_context* cpu_context,
                                 void* dispatcher_context);

struct x86_seh_registration {
  x86_seh_registration* next;
  x86_frame_handler_t* handler;
};

using x64_frame_handler_t =
    ExceptionDisposition(exception_record* exception_record,
                         byte* frame_ptr,
                         x64_cpu_context*,
                         void* dispatcher_context);

}  // namespace ktl::crt::exc_engine::win
