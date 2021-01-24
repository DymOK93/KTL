#include <bugcheck.h>
#include <throw.h>
#include <seh.hpp>

namespace ktl::crt::exc_engine::x64 {
namespace fh3 {
struct function_context {
  int32_t state;
  int32_t home_block_index;
};
}  // namespace fh3

static win::ExceptionDisposition frame_handler(
    win::exception_record*,
    byte* frame_ptr,
    win::x64_cpu_context*,
    x64::dispatcher_context* dispatcher_context) noexcept;


EXTERN_C win::ExceptionDisposition __CxxFrameHandler3(
    win::exception_record* exception_record,
    byte* frame_ptr,
    win::x64_cpu_context* cpu_ctx,
    dispatcher_context* dispatcher_ctx) noexcept {
  if (exception_record) {
    crt_critical_failure_if_not(
        exception_record->flags.has_any_of(win::ExceptionFlag::Unwinding));
    return win::ExceptionDisposition::ContinueSearch;
  }
  return frame_handler(exception_record, frame_ptr, cpu_ctx, dispatcher_ctx);
}

EXTERN_C ktl::crt::exc_engine::win::ExceptionDisposition __GSHandlerCheck_EH(
    ktl::crt::exc_engine::win::exception_record* exception_record,
    ktl::byte* frame_ptr,
    win::x64_cpu_context* cpu_ctx,
    dispatcher_context* ctx) noexcept {
  /*No cookie check :( */
  return __CxxFrameHandler3(exception_record, frame_ptr, cpu_ctx, ctx);
}

static win::ExceptionDisposition frame_handler(
    win::exception_record*,
    byte* frame_ptr,
    win::x64_cpu_context*,
    x64::dispatcher_context* dispatcher_context) noexcept {
  if (dispatcher_context->cookie == &x64::rethrow_probe_cookie) {
    return win::ExceptionDisposition::CxxHandler;
  }
  if (dispatcher_context->cookie != &x64::unwind_cookie) {
    return win::ExceptionDisposition::ContinueSearch;
  }

  auto* throw_frame = dispatcher_context->throw_frame;

  auto& catch_info{throw_frame->catch_info};
  auto* ctx{
      reinterpret_cast<x64::fh3::function_context*>(catch_info.unwind_context)};

  const byte* image_base = dispatcher_context->pdata->image_base();

  const auto eh_info_rva{*static_cast<
      const relative_virtual_address<const x64::function_eh_info>*>(
          dispatcher_context->extra_data)};

  const auto* eh_info{image_base + eh_info_rva};
  const auto try_blocks{image_base + eh_info->try_blocks};

  byte* primary_frame_ptr = catch_info.primary_frame_ptr;
  int32_t state;
  int32_t home_block_index;

  if (primary_frame_ptr < frame_ptr) {
    auto pc_rva{make_rva(reinterpret_cast<const byte*>(throw_frame->mach.rip),
                         image_base)};

    auto regions = image_base + eh_info->regions;
    state = regions[eh_info->region_count - 1].state;
    for (uint32_t i = 1; i != eh_info->region_count; ++i) {
      if (pc_rva < regions[i].first_ip) {
        state = regions[i - 1].state;
        break;
      }
    }

    home_block_index = eh_info->try_block_count - 1;
  } else {
    state = ctx->state;
    home_block_index = ctx->home_block_index;
  }

  int32_t funclet_low_state = -1;
  if (primary_frame_ptr == frame_ptr) {
    home_block_index = -1;
  } else {
    // Identify the funclet we're currently in. If we're in a catch
    // funclet, locate the primary frame ptr and cache everything.
    for (; home_block_index > -1; --home_block_index) {
      const auto& try_block{try_blocks[home_block_index]};
      if (try_block.try_high < state && state <= try_block.catch_high) {
        if (primary_frame_ptr < frame_ptr) {
          const auto* catch_handlers{image_base + try_block.catch_handlers};
          for (int32_t idx = 0; idx < try_block.catch_count; ++idx) {
            const auto& catch_handler{catch_handlers[idx]};
            if (catch_handler.handler == dispatcher_context->fn->begin) {
              const auto* node{frame_ptr + catch_handler.node_offset};
              primary_frame_ptr = node->primary_frame_ptr;
              crt_critical_failure_if_not(primary_frame_ptr >= frame_ptr);
              break;
            }
          }
        }
        funclet_low_state = try_block.try_low;
        break;
      }
    }

    if (primary_frame_ptr < frame_ptr)
      primary_frame_ptr = frame_ptr;
  }

  int32_t target_state{funclet_low_state};

  const auto* throw_info{catch_info.get_throw_info()};

  const x64::catch_handler* target_catch_handler = nullptr;
  for (int32_t idx = home_block_index + 1;
       !target_catch_handler && idx < eh_info->try_block_count; ++idx) {
    const auto& try_block{try_blocks[idx]};

    if (try_block.try_low <= state && state <= try_block.try_high) {
      if (try_block.try_low < funclet_low_state) {
        continue;
      }
      if (!throw_info) {
        probe_for_exception(*dispatcher_context->pdata, *throw_frame);
        throw_info = catch_info.get_throw_info();
      }

      const auto* handlers{image_base + try_block.catch_handlers};
      for (int32_t handler_idx = 0; handler_idx < try_block.catch_count;
           ++handler_idx) {
        const auto& catch_block{handlers[handler_idx]};

        if (!process_catch_block(
                image_base, catch_block.adjectives,
                image_base + catch_block.type_desc,
                primary_frame_ptr + catch_block.catch_object_offset,
                catch_info.get_exception_object(), *throw_info)) {
          continue;
        }

        target_state = try_block.try_low - 1;
        target_catch_handler = &catch_block;
        dispatcher_context->handler = image_base + catch_block.handler;

        catch_info.primary_frame_ptr = primary_frame_ptr;
        ctx->home_block_index = home_block_index;
        ctx->state = try_block.try_low - 1;
        break;
      }
    }
  }

  if (home_block_index == -1 && !target_catch_handler &&
      eh_info->eh_flags.has_any_of(x64::EhFlag::IsNoexcept)) {
    // call std::terminate
    crt_critical_failure();
  }

  crt_critical_failure_if_not(target_state >= funclet_low_state);

   x64::unwind_graph_edge const* unwind_graph =
      image_base + eh_info->unwind_graph;
  while (state > target_state) {
    const auto& edge{unwind_graph[state]};
    state = edge.next;
    if (edge.cleanup_handler) {
      byte const* funclet = image_base + edge.cleanup_handler;
      using fn_type = uintptr_t(byte const* funclet, byte const* frame_ptr);
      (void)((fn_type*)funclet)(funclet, frame_ptr);
    }
  }

  return win::ExceptionDisposition::CxxHandler;
}
}  // namespace ktl::crt::exc_engine::x64
