#include <internal_typedefs.h>

namespace ktl::crt::exc_engine {
disposition_t handle_stack_frame_impl(const record_t& exc_rec,
                                      void* frame_ptr,
                                      const cpu_context_t& cpu_context,
                                      const dispatcher_context_t& dc);

disposition_t handle_stack_frame(record_t* exc_rec,
                                 void* frame_ptr,
                                 cpu_context_t* cpu_context,
                                 void* dispatcher_context) {
  const auto* dc{static_cast<dispatcher_context_t*>(dispatcher_context)};
  return handle_stack_frame_impl(*exc_rec, frame_ptr, *cpu_context, *dc);
}
}  // namespace ktl::crt::exc_engine

