#pragma once
#include <../basic_types.h>

namespace ktl::crt::exc_engine {
struct dispatcher_context;

win::ExceptionDisposition frame_handler3(
    ktl::byte* frame_ptr,
    dispatcher_context* dispatcher_context);

win::ExceptionDisposition frame_handler4(
    ktl::byte* frame_ptr,
    dispatcher_context* dispatcher_context);
}  // namespace ktLLcrt::exc_engine
