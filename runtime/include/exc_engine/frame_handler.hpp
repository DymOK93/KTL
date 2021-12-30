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
  
void gs_handler(byte* frame_ptr, dispatcher_context* dispatcher_context);
}  // namespace ktl::crt::exc_engine::x64

