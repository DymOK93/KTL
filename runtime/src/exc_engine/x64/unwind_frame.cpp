#include <../exc_engine_interface.h>
#include <internal_typedefs.h>
#include <invoke_funclet.h>
#include <rtl_import.h>

namespace ktl::crt::exc_engine {
void unwind_stack_frame(const function_descriptor& fn_desc,
                        void* stack_frame,
                        int target_state,
                        const dispatcher_context_t& dc);

const function_descriptor& get_function_descriptor(
    const dispatcher_context_t& dc);
frame_pointers& get_frame_pointers(const function_descriptor& fn_desc,
                                   void* stack_frame);
int get_state_from_ip_reg(const function_descriptor& fn_desc,
                          const dispatcher_context_t& dc);

[[noreturn]] void notify_exc_engine_fatal_error();

// namespace aux_ {
// struct continuation {
//  static void invoke(continuation* const, exception_registration const&);
//};
//}  // namespace aux_
// typedef aux_::continuation* continuation_ft;
//
// namespace aux_ {
// struct funclet {
//  static continuation_ft invoke(funclet* const, exception_registration
//  const&);
//};
//}  // namespace aux_

const function_descriptor& get_function_descriptor(
    const dispatcher_context_t& dc) {
  auto* raw_desc{
      reinterpret_cast<const function_descriptor* const>(
          dc.ImageBase +
          *reinterpret_cast<unsigned int*>(dc.HandlerData))  // Why not size_t?
  };
  return *raw_desc;
}

disposition_t handle_stack_frame_impl(
    [[maybe_unused]] const record_t& exc_rec,
    [[maybe_unused]] void* frame_ptr,
    [[maybe_unused]] const cpu_context_t& cpu_context,
    [[maybe_unused]] const dispatcher_context_t& dc) {
  // the function this handler is invoked on behalf of
  const auto& fn_desc(get_function_descriptor(dc));

  //// proceed the unwind and return
  if (exc_rec.ExceptionFlags & EXCEPTION_UNWIND) {
    if (fn_desc.unwind_array_size) {
     // if ((exc_rec.ExceptionFlags & EXCEPTION_TARGET_UNWIND))// &&
        //  STATUS_UNWIND_CONSOLIDATE == exc_rec.ExceptionCode) 
      {
        // unwinding the target frame
        if (ARRAYSIZE_EXCPTR_UNW == exc_rec.NumberParameters &&
            EXCEPTION_OPCODE_STACK_CONSOLIDATE ==
                exc_rec.ExceptionInformation[EXCPTR_OPCODE]) {
          unwind_stack_frame(
              fn_desc,
              reinterpret_cast<void*>(
                  exc_rec.ExceptionInformation[EXCPTR_UNW_FUNCTION_FRAME]),
              static_cast<int>(
                  exc_rec.ExceptionInformation[EXCPTR_UNW_TARGET_STATE]),
              dc);
        }
     // } else {
        // unwinding the nested frames
        /* funcframe_ptr_t func_frame =
             eh_state::function_frame(fn_desc, frame_ptr, dc);
         ehstate_t target_state = -1;
         ehstate_t const state = eh_state::from_dc(func_dsc, dc);
         for (try_rev_iterator try_block(*func_dsc, dc->ImageBase);
              try_block.valid(); try_block.next()) {
           if (state > try_block->high_level &&
               state <= try_block->catch_level) {
             target_state = try_block->high_level;
             break;
           }
         }
         unwind_stack_frame(fn_desc, func_frame, target_state, dc);*/
      }
    }

    //  return ::ExceptionContinueSearch;
    //}

    // else find and invoke the handler

    // find_matching_catch_block(exc_rec, func_dsc, frame, dc);

    //  does not return if matching catch-block has been found (if the standard
    //  NT exception dispatcher is used) or returns and delegates the unwinding
    //  duties to a custom dispatching routine
  }
  return disposition_t::ExceptionContinueSearch;
}

void unwind_stack_frame(const function_descriptor& fn_desc,
                        void* stack_frame,
                        int target_state,
                        const dispatcher_context_t& dc) {
  int current_state = get_frame_pointers(fn_desc, stack_frame).current_state;
  if (current_state < -1) {  // invalid state
    current_state = get_state_from_ip_reg(fn_desc, dc);
  }
  auto& unwind_array{fn_desc.unwind_array};

  // unwind_iterator unwind_entry(func_dsc., dc.ImageBase);
  while (current_state > -1 && current_state != target_state) {
    auto& unwind_entry{unwind_array[current_state]};
    current_state = unwind_entry.prev_state;
    auto& action{unwind_entry.action};

    // unwind_action_iterator unwind_action(*unwind_entry, dc.ImageBase);
    // if (unwind_action.valid()) {
    get_frame_pointers(fn_desc, stack_frame).current_state = current_state;
    __try {
      exc_engine_invoke_funclet(action, stack_frame);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      // notify_exc_engine_fatal_error();
      NT_ASSERT(false);
    }
    // }
  }
  get_frame_pointers(fn_desc, stack_frame).current_state = current_state;
}

frame_pointers& get_frame_pointers(const function_descriptor& fn_desc,
                                   void* stack_frame) {
  auto* fp{reinterpret_cast<frame_pointers*>(
      reinterpret_cast<size_t>(stack_frame) + fn_desc.frame_ptrs)};
  return *fp;
}

int get_state_from_ip_reg([[maybe_unused]] const function_descriptor& fn_desc,
                          const dispatcher_context_t& dc) {
  uint64_t ip{dc.ControlPc};
  if (dc.ControlPcIsUnwound) {
    ip -= 4;  //  rewind the forwarded execution point
  }
  int state = -1;
  // auto ip_rva = ip - dc.ImageBase;
  /*ip2state_iterator ip2state(fn_desc, dc.ImageBase);
  for (; ip2state.valid(); ip2state.next()) {
    if (ip_rva < ip2state->ip) {
      break;
    }
  }
  if (ip2state.prev()) {
    state = ip2state->state;
  }*/
  return state;
}

[[noreturn]] void notify_exc_engine_fatal_error() {
  KeBugCheck(SYSTEM_THREAD_EXCEPTION_NOT_HANDLED);
}
}  // namespace ktl::crt::exc_engine