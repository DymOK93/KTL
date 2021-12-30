#pragma once
#include "seh.hpp"

#include <../basic_types.hpp>
#include <../crt_attributes.hpp>

namespace ktl::crt::exc_engine::x64 {
struct dispatcher_context;

win::ExceptionDisposition frame_handler3(
    byte* frame_ptr,
    dispatcher_context* dispatcher_context);

win::ExceptionDisposition frame_handler4(
    byte* frame_ptr,
    dispatcher_context* dispatcher_context);

EXTERN_C void __GSHandlerCheckCommon(byte* frame_ptr,
                                     dispatcher_context* ctx,
                                     const void* handler_data) noexcept;
}  // namespace ktl::crt::exc_engine
