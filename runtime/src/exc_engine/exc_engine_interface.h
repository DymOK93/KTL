#pragma once
#include <basic_types.h>

#include <ntddk.h>

namespace ktl::crt::exc_engine {
using record_t = EXCEPTION_RECORD;
using cpu_context_t = CONTEXT;
using disposition_t = EXCEPTION_DISPOSITION;

disposition_t handle_stack_frame(record_t* exc_rec,
                                 void* frame_ptr,
                                 cpu_context_t* context,
                                 void* dispatcher_context);
}  // namespace ktl::crt::exc_engine