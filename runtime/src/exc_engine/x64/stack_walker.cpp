#pragma once
#include <basic_types.h>
#include <crt_attributes.h>
#include <internal_typedefs.h>
#include <irql.h>

using ktl::crt::exc_engine::cpu_context_t;
using ktl::crt::exc_engine::dispatcher_context_t;
using ktl::crt::exc_engine::disposition_t;
using ktl::crt::exc_engine::record_t;

namespace ktl::crt::exc_engine {
ALIGN(16) struct cpu_reg_context {
  uint64_t Rax;
  uint64_t Rcx;
  uint64_t Rdx;
  uint64_t Rbx;
  uint64_t Rsp;
  uint64_t Rbp;
  uint64_t Rsi;
  uint64_t Rdi;
  uint64_t R8;
  uint64_t R9;
  uint64_t R10;
  uint64_t R11;
  uint64_t R12;
  uint64_t R13;
  uint64_t R14;
  uint64_t R15;

  M128A Xmm0;
  M128A Xmm1;
  M128A Xmm2;
  M128A Xmm3;
  M128A Xmm4;
  M128A Xmm5;
  M128A Xmm6;
  M128A Xmm7;
  M128A Xmm8;
  M128A Xmm9;
  M128A Xmm10;
  M128A Xmm11;
  M128A Xmm12;
  M128A Xmm13;
  M128A Xmm14;
  M128A Xmm15;

  uint64_t Rip;
  uint32_t MxCsr;
  uint16_t SegCs;
  uint16_t SegDs;
  uint16_t SegEs;
  uint16_t SegFs;
  uint16_t SegGs;
  uint16_t SegSs;
  uint32_t EFlags;
  uint32_t fill_;

  cpu_reg_context& operator=(const cpu_context_t& ctx) {
    Rax = ctx.Rax;
    Rcx = ctx.Rcx;
    Rdx = ctx.Rdx;
    Rbx = ctx.Rbx;
    Rsp = ctx.Rsp;
    Rbp = ctx.Rbp;
    Rsi = ctx.Rsi;
    Rdi = ctx.Rdi;
    R8 = ctx.R8;
    R9 = ctx.R9;
    R10 = ctx.R10;
    R11 = ctx.R11;
    R12 = ctx.R12;
    R13 = ctx.R13;
    R14 = ctx.R14;
    R15 = ctx.R15;
    Xmm0 = ctx.Xmm0;
    Xmm1 = ctx.Xmm1;
    Xmm2 = ctx.Xmm2;
    Xmm3 = ctx.Xmm3;
    Xmm4 = ctx.Xmm4;
    Xmm5 = ctx.Xmm5;
    Xmm6 = ctx.Xmm6;
    Xmm7 = ctx.Xmm7;
    Xmm8 = ctx.Xmm8;
    Xmm9 = ctx.Xmm9;
    Xmm10 = ctx.Xmm10;
    Xmm11 = ctx.Xmm11;
    Xmm12 = ctx.Xmm12;
    Xmm13 = ctx.Xmm13;
    Xmm14 = ctx.Xmm14;
    Xmm15 = ctx.Xmm15;
    Rip = ctx.Rip;
    MxCsr = ctx.MxCsr;
    SegCs = ctx.SegCs;
    SegDs = ctx.SegDs;
    SegEs = ctx.SegEs;
    SegFs = ctx.SegFs;
    SegGs = ctx.SegGs;
    SegSs = ctx.SegSs;
    EFlags = ctx.EFlags;

    return *this;
  }
};
}  // namespace ktl::crt::exc_engine

EXTERN_C void exc_engine_execute_handler(
    record_t* exc_rec,
    void* frame,
   cpu_context_t* ctx,
    dispatcher_context_t* dsp_ctx);

EXTERN_C void exc_engine_restore_context(
    ktl::crt::exc_engine::cpu_reg_context* ctx);

// namespace ktl::crt::exc_engine {
//////////////////////////////////////////////////////////
//////////  EH stack dispatching/unwinding low-level routines
//////////////////////////////////////////////////////////
// enum { STACK_MISALIGNMENT = (1 << 3) - 1 };
//
// NOINLINE bool stack_walk_impl(record_t& original_exc_rec,
//                              record_t& unwind_exc_rec,
//                              cpu_req_context& restore_ctx,
//                              int walker_scope_depth) {
//  // the data used during both the dispatching phase and the unwinding one:
//  ::UNWIND_HISTORY_TABLE uht = {0};
//  cpu_context_t ctx1;
//  cpu_context_t ctx2;
//  cpu_context_t* prev_ctx = &ctx2;
//  record_t* current_exc_rec = &original_exc_rec;
//  size_t current_frame = 0;
//  ::RUNTIME_FUNCTION* current_rtf = 0;
//  ::UNWIND_HISTORY_TABLE* current_uht = &uht;
//  image_base_t image_base = 0;
//  int scope_index = 0;
//
//  // getting the context raised the exception into the ctx1
//  verify_max_irql(DISPATCH_LEVEL);  // RtlCaptureContext()
//  RtlCaptureContext(&ctx1);
//  do {
//    verify_max_irql(DISPATCH_LEVEL);  // RtlLookupFunctionEntry()
//    current_rtf = RtlLookupFunctionEntry(ctx1.Rip, &image_base, current_uht);
//    if (!current_rtf) {
//      unwind_exc_rec.ExceptionCode =
//          eh::EXCEPTION_CODE_STACKWALKER_UNEXPECTED_LEAF_ENTRY;
//      unwind_exc_rec.ExceptionFlags |= EXCEPTION_NONCONTINUABLE;
//      unwind_exc_rec.ExceptionRecord = &original_exc_rec;
//      unwind_exc_rec.NumberParameters = 0;
//      eh::aux_::raise_exception(unwind_exc_rec);
//    }
//    verify_max_irql(DISPATCH_LEVEL);    // RtlVirtualUnwind()
//    RtlVirtualUnwind(UNW_FLAG_NHANDLER  // skip the frame_handler searching
//                     ,
//                     image_base, ctx1.Rip, current_rtf, &ctx1, 0,
//                     &current_frame, 0);
//  } while (walker_scope_depth-- > 0);
//  original_exc_rec.ExceptionAddress = reinterpret_cast<void*>(ctx1.Rip);
//  // at this point the throwed function context resides in the 'ctx1'
//
//  // the data used during the dispatching phase:
//  size_t nested_frame = 0;
//  cpu_context_t const* original_ctx = &ctx1;
//  *prev_ctx = *original_ctx;
//
//  // the data used during the unwinding phase:
//  bool unwind_phase = false;
//  size_t unwind_target_frame = 0;
//  cpu_context_t* current_ctx = 0;
//  if (eh::EXCEPTION_CODE_CPP == original_exc_rec.ExceptionCode &&
//      EXCEPTION_OPCODE_THROW ==
//          original_exc_rec.ExceptionInformation[EXCPTR_OPCODE]) {
//    unwind_exc_rec.ExceptionCode = eh::EXCEPTION_CODE_CPP;
//    unwind_exc_rec.ExceptionInformation[EXCPTR_OPCODE] =
//        EXCEPTION_OPCODE_STACKWALKER_UNWIND;
//    original_exc_rec.ExceptionInformation[EXCPTR_THR_UNWIND_EXCREC] =
//        reinterpret_cast< ::ULONG_PTR>(
//            &unwind_exc_rec);  // here the frame handler is to save the
//                               // unwinding info if any
//    original_exc_rec.ExceptionInformation[EXCPTR_FLAGS] |=
//        EXCEPTION_FLAG_STACKWALKER_UNWIND;
//  }
//
//  //// following is the loop traversing the stack frames for:
//  //// 1- using the current_exc_rec=original_exc_rec until the frame
//  responsible
//  /// of the exception handling is found, and then / 2- using the
//  /// current_exc_rec=unwind_exc_rec to perform unwinding tasks and the stack
//  /// consolidation
//  while (1)  // the frame-traversing loop
//  {
//    if (unwind_phase) {
//      *prev_ctx = *current_ctx;  // fill in the *prev_ctx with the current
//                                 // context for to be rewound to the previous
//    }
//
//    verify_max_irql(DISPATCH_LEVEL);  // RtlLookupFunctionEntry()
//    current_rtf =
//        RtlLookupFunctionEntry(prev_ctx->Rip, &image_base, current_uht);
//    if (!current_rtf) {
//      if (0 == prev_ctx->Rip) {
//        goto stack_walker_unexpected;
//      }
//      // leaf entry
//      prev_ctx->Rip = *reinterpret_cast< size_t*>(
//          prev_ctx->Rsp);              // pop the ret address from the stack
//      prev_ctx->Rsp += sizeof(void*);  // adjust the stack
//      continue;  // the leaf entry have not anything to deal with unwinding
//    } else {
//      // nested entry
//      void* frame_handler_data = 0;
//      size_t control_pc =
//          prev_ctx
//              ->Rip;  // save the currently reached frame instruction pointer
//
//      verify_max_irql(DISPATCH_LEVEL)  // ::RtlVirtualUnwind()
//      PEXCEPTION_ROUTINE frame_handler = RtlVirtualUnwind(
//          unwind_phase ? UNW_FLAG_UHANDLER
//                       : UNW_FLAG_EHANDLER  // let it find the frame_handler
//          ,
//          image_base, control_pc, current_rtf, prev_ctx, &frame_handler_data,
//          &current_frame, 0);
//
//      if (current_frame & STACK_MISALIGNMENT) {
//        goto stack_walker_bad_stack;
//      }
//
//      if (frame_handler)  // an exception-aware frame is found, let's check
//      its'
//                          // intentions
//      {
//      stack_walker_restart_frame_loop_for_collided_unwind:
//
//        // preparing the frame handler for invoking
//
//        dispatcher_context_t dsp_ctx = {0};
//        dsp_ctx.ControlPc = control_pc;
//        dsp_ctx.ImageBase = image_base;
//        dsp_ctx.FunctionEntry = current_rtf;
//        dsp_ctx.EstablisherFrame = current_frame;
//        dsp_ctx.TargetIp = 0;
//        dsp_ctx.ContextRecord = unwind_phase ? current_ctx : prev_ctx;
//        dsp_ctx.LanguageHandler = frame_handler;
//        dsp_ctx.HandlerData = frame_handler_data;
//        dsp_ctx.HistoryTable = current_uht;
//        dsp_ctx.ScopeIndex = scope_index;
//
//        if (unwind_phase) {
//          if (current_frame > unwind_target_frame) {
//            goto stack_walker_bad_stack;
//          }
//
//          if (current_frame == unwind_target_frame) {
//            current_exc_rec->ExceptionFlags |= EXCEPTION_TARGET_UNWIND;
//          }
//        }
//
//        // do invoke the frame handler
//        int disposition = _CPPRTL_execute_frame_handler(
//            current_exc_rec, reinterpret_cast<void*>(current_frame),
//            unwind_phase ? current_ctx : const_cast<
//            cpu_context_t*>(original_ctx), &dsp_ctx);
//
//        // adjust the environment after the frame handler
//        if (nested_frame == current_frame) {
//          current_exc_rec->ExceptionFlags &= ~EXCEPTION_NESTED_CALL;
//          nested_frame = 0;
//        }
//        if (unwind_phase) {
//          current_exc_rec->ExceptionFlags &=
//              ~(EXCEPTION_COLLIDED_UNWIND | EXCEPTION_TARGET_UNWIND);
//        }
//
//        // check the frame disposition
//        switch (disposition) {
//          case ExceptionContinueExecution:  // expected at the dispatching
//            if (current_exc_rec->ExceptionFlags & EXCEPTION_NONCONTINUABLE) {
//              goto stack_walker_noncontinuable;
//            }
//            goto stack_walker_continue_execution;
//            break;
//
//          case ExceptionNestedException:  // expected only at the dispatching
//            current_exc_rec->ExceptionFlags |= EXCEPTION_NESTED_CALL;
//            if (dsp_ctx.EstablisherFrame > nested_frame) {
//              nested_frame = dsp_ctx.EstablisherFrame;
//            }
//            continue;
//
//          case ExceptionCollidedUnwind:  // expected both at the dispatching
//          and
//                                         // the unwinding, let's undertake the
//                                         // over-stepping
//            // the upper-frame dispatching loop context is expected to be
//            placed
//            // at the moment in our dsp_ctx, so let's continue the current
//            loop
//            // from the point reached by the predecessor
//            control_pc = dsp_ctx.ControlPc;
//            image_base = dsp_ctx.ImageBase;
//            current_rtf = dsp_ctx.FunctionEntry;
//            current_frame = dsp_ctx.EstablisherFrame;
//            frame_handler = dsp_ctx.LanguageHandler;
//            frame_handler_data = dsp_ctx.HandlerData;
//            current_uht = dsp_ctx.HistoryTable;
//            scope_index = dsp_ctx.ScopeIndex;
//
//            if (!unwind_phase) {
//              // the frame handler invoked at the dispatching phase is
//              expected
//              // to keep the previous frame context within the supplied
//              // dispatcher context
//              *prev_ctx = *dsp_ctx.ContextRecord;
//              prev_ctx->Rip = control_pc;
//            } else {
//              // the frame handler invoked at the unwinding phase is expected
//              to
//              // keep the current frame context within the supplied dispatcher
//              // context so we are to undertake the unwind to the previous
//              frame
//              // before the loop is bacame restarted:
//              *current_ctx = *prev_ctx = *dsp_ctx.ContextRecord;
//              RtlVirtualUnwind(
//                  UNW_FLAG_NHANDLER, image_base, control_pc, current_rtf,
//                  prev_ctx,
//                  &dsp_ctx
//                       .HandlerData  // these are already saved in tmp vars so
//                  ,
//                  &dsp_ctx.EstablisherFrame  // we may junk the dsp_ctx
//                  ,
//                  0);
//              current_exc_rec->ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
//            }
//            goto stack_walker_restart_frame_loop_for_collided_unwind;
//            continue;
//
//          case ExceptionContinueSearch:  // expected both at the dispatching
//          and
//                                         // the unwinding
//            if (!unwind_phase)  //  here to check if the stack unwind should
//            be
//                                //  started or even proceed to the stack
//                                //  consolidation
//            {
//              if  // the following flags are expected to be set only at the
//                  // eh_engine::stack_unwind():
//                  (STATUS_UNWIND_CONSOLIDATE == unwind_exc_rec.ExceptionCode
//                  &&
//                   EXCEPTION_OPCODE_STACK_CONSOLIDATE ==
//                       unwind_exc_rec.ExceptionInformation[EXCPTR_OPCODE]) {
//                // make the preparations for the unwind phase:
//                original_ctx = 0;
//                current_ctx = &ctx1;  // original context is expected to be
//                here
//                                      // at the moment
//                prev_ctx = &ctx2;
//
//                unwind_target_frame =
//                    unwind_exc_rec
//                        .ExceptionInformation[EXCPTR_UNW_TARGET_FRAME];
//                unwind_exc_rec.ExceptionFlags |=
//                    EXCEPTION_UNWINDING;  // 'EXCEPTION_EXIT_UNWIND' isn't
//                                          // expected 'cos we have the target
//                                          // frame
//                unwind_exc_rec.ExceptionAddress =
//                    reinterpret_cast<void*>(prev_ctx->Rip);
//
//                current_exc_rec = &unwind_exc_rec;
//                nested_frame = 0;
//                unwind_phase = true;  // the unwind phase can be started only
//                by
//                                      // these eh-lib frame handler
//                continue;  // follow to the unwinding phase of the stack
//                walking
//              }
//            } else  // if the unwinding is at a progress:
//            {
//              if (current_frame ==
//                  unwind_target_frame)  // the target frame has been unwound
//                  and
//                                        // we are to perform the stack
//                                        // consolidation
//              {
//                goto stack_walker_restore_context;
//              }
//              // move the current context up
//              cpu_context_t* tmp_ctx = current_ctx;
//              current_ctx = prev_ctx;
//              prev_ctx = tmp_ctx;
//            }
//            continue;
//
//          default:
//            goto stack_walker_invalid_disposition;  // the frame handler is
//            not
//                                                    // expected to return
//                                                    these
//                                                    // values
//            break;
//        };
//
//      } else if (unwind_phase)  // there are no handler but we are to check
//      the
//                                // target frame and to move the context up
//      {
//        if (current_frame ==
//            unwind_target_frame)  // the target frame has been unwound and we
//                                  // are to perform the stack consolidation
//        {
//          goto stack_walker_restore_context;
//        }
//        // move the current context up
//        cpu_context_t* tmp_ctx = current_ctx;
//        current_ctx = prev_ctx;
//        prev_ctx = tmp_ctx;
//      }
//    }
//  }
//
// stack_walker_restore_context:
//  restore_ctx = *current_ctx;
//  return true;
//
// stack_walker_continue_execution:
//  return false;
//
// stack_walker_noncontinuable:
//  unwind_exc_rec.ExceptionCode = STATUS_NONCONTINUABLE_EXCEPTION;
//  unwind_exc_rec.ExceptionFlags |= EXCEPTION_NONCONTINUABLE;
//  unwind_exc_rec.ExceptionRecord = &original_exc_rec;
//  unwind_exc_rec.NumberParameters = 0;
//  eh::aux_::raise_exception(unwind_exc_rec);
//
// stack_walker_invalid_disposition:
//  unwind_exc_rec.ExceptionCode = STATUS_INVALID_DISPOSITION;
//  unwind_exc_rec.ExceptionFlags |= EXCEPTION_NONCONTINUABLE;
//  unwind_exc_rec.ExceptionRecord = &original_exc_rec;
//  unwind_exc_rec.NumberParameters = 0;
//  eh::aux_::raise_exception(unwind_exc_rec);
//
// stack_walker_bad_stack:
//  unwind_exc_rec.ExceptionCode = eh::EXCEPTION_CODE_STACKWALKER_BAD_STACK;
//  unwind_exc_rec.ExceptionFlags |= EXCEPTION_NONCONTINUABLE;
//  unwind_exc_rec.ExceptionRecord = &original_exc_rec;
//  unwind_exc_rec.NumberParameters = 0;
//  eh::aux_::raise_exception(unwind_exc_rec);
//
// stack_walker_unexpected:
//  unwind_exc_rec.ExceptionCode =
//      eh::EXCEPTION_CODE_STACKWALKER_UNEXPECTED_RETURN;
//  unwind_exc_rec.ExceptionFlags |= EXCEPTION_NONCONTINUABLE;
//  unwind_exc_rec.ExceptionRecord = &original_exc_rec;
//  unwind_exc_rec.NumberParameters = 0;
//  eh::aux_::raise_exception(unwind_exc_rec);
//
//  return false;  // could anyone achieve here ?
//}
//
// NOINLINE void stack_walk(record_t& original_exc_rec) {
//  // do NOT use objects with the destructor semantic at this scope because of
//  // 'XXRestoreContext()' just cuts the stack up the data to be used in the
//  // stack_walk_impl()
//  cpu_req_context restore_ctx;                  // keep at this scope for
//  using with
//                                            // '_CPPRTL_restore_context()'
//  record_t unwind_exc_rec = {0};  // ^
//
//  if (stack_walk_impl(original_exc_rec, unwind_exc_rec, restore_ctx, 1)) {
//    // the stack_walk_impl() has got the stack unwound and has filled the
//    'ctx1'
//    // and the 'unwind_exc_rec' with the proper consolidation data
//    restore_ctx.Rip = (reinterpret_cast< size_t (*)(record_t*)>(
//        unwind_exc_rec.ExceptionInformation[EXCPTR_UNW_CALLBACK_ADDR]))(
//        &unwind_exc_rec);
//    // just doesn't return into this scope
//    _CPPRTL_restore_context(
//        &restore_ctx);  // the similar to RtlRestoreContext(&ctx, 0);
//  }
//  // otherwise the stack_walk_impl() requests for the stack_walk() caller
//  // continuation
//  return;
//}
//}  // namespace ktl::crt::exc_engine
