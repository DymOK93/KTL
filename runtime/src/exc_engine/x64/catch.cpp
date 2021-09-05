#include <bugcheck.hpp>
#include <catch.hpp>
#include <seh.hpp>
#include <throw.hpp>

EXTERN_C ktl::crt::exc_engine::symbol __ImageBase;

namespace ktl::crt::exc_engine::x64 {
void* catch_info::get_exception_object() const noexcept {
  if (throw_info_if_owner) {
    return exception_object_or_link;
  }
  if (exception_object_or_link) {
    const auto* other =
        static_cast<const catch_info*>(exception_object_or_link);
    return other->exception_object_or_link;
  }
  return nullptr;
}

const throw_info* catch_info::get_throw_info() const noexcept {
  if (exception_object_or_link) {
    if (!throw_info_if_owner) {
      const auto* other =
          static_cast<const catch_info*>(this->exception_object_or_link);
      return other->throw_info_if_owner;
    }
    return throw_info_if_owner;
  }
  return nullptr;
}

static void copy_to_catch_block(const byte* image_base,
                                const catchable_type* catchable,
                                void* catch_var,
                                void* exception_object) noexcept {
  if (catchable->properties.has_any_of(CatchableProperty::IsSimpleType)) {
    memcpy(catch_var, exception_object, catchable->size);

  } else if (!catchable->copy_fn) {
    const auto address{
        catchable->offset.apply(reinterpret_cast<uintptr_t>(exception_object))};
    memcpy(catch_var, reinterpret_cast<void*>(address), catchable->size);

  } else if (catchable->properties.has_any_of(
                 CatchableProperty::HasVirtualBase)) {
    auto* raw_copy_ctor{const_cast<byte*>(image_base + catchable->copy_fn)};
    auto* copy_ctor{reinterpret_cast<copy_ctor_virtual_base_t*>(raw_copy_ctor)};
    copy_ctor(catch_var, exception_object, 1 /* is most derived */); 

  } else {
    auto* raw_copy_ctor{const_cast<byte*>(image_base + catchable->copy_fn)};
    auto* copy_ctor{reinterpret_cast<copy_ctor_t*>(raw_copy_ctor)};
    copy_ctor(catch_var, exception_object);
  }
}

static void transfer_to_catch_block(const byte* image_base,
                                    flag_set<CatchFlag> adjectives,
                                    const catchable_type* catchable,
                                    void* catch_var,
                                    void* exception_object) noexcept {
  if (!catchable->properties.has_any_of(CatchableProperty::ByReferenceOnly) ||
      adjectives.has_any_of(CatchFlag::IsReference)) {
    if (!adjectives.has_any_of(CatchFlag::IsReference)) {
      copy_to_catch_block(image_base, catchable, catch_var, exception_object);
    } else {
      auto* catch_var_holder{static_cast<void**>(catch_var)};
      *catch_var_holder = exception_object;
    }
  }
}

static bool process_catch_block_unchecked(
    const byte* image_base,
    flag_set<CatchFlag> adjectives,
    const type_info* match_type,
    void* catch_var,
    void* exception_object,
    const catchable_type_list* catchable_list) noexcept {
  const auto* catchables = catchable_list->types;

  for (uint32_t idx = 0; idx < catchable_list->count; ++idx) {
    const catchable_type* catchable = image_base + catchables[idx];
    if (const type_info* type_descriptor = image_base + catchable->desc;
        type_descriptor == match_type) {
      transfer_to_catch_block(image_base, adjectives, catchable, catch_var,
                              exception_object);
      return true;
    }
  }
  return false;
}

bool process_catch_block(const byte* image_base,
                         flag_set<CatchFlag> adjectives,
                         const type_info* match_type,
                         void* catch_var,
                         void* exception_object,
                         const throw_info& throw_info) noexcept {
  if (adjectives.has_any_of(CatchFlag::IsEllipsis)) {
    return true;
  }

  /*
   * Returns false, if throw has one of IsConst, IsVolatile, or IsUnaligned
   * flags, and the catch block hasn't
   */
  if (throw_info.attributes.bit_intersection<
          ThrowFlag::IsConst, ThrowFlag::IsVolatile, ThrowFlag::IsUnaligned>() &
      adjectives.bit_negation()) {
    return false;
  }

  return process_catch_block_unchecked(image_base, adjectives, match_type,
                                       catch_var, exception_object,
                                       image_base + throw_info.catchables);
}

EXTERN_C win::ExceptionDisposition __cxx_seh_frame_handler(
    win::exception_record* exception_record,
    byte*,
    win::x64_cpu_context*,
    void*) {
  if (exception_record != &win::exc_record_cookie) {
    verify_seh(exception_record->code, exception_record->address,
               exception_record->flags.value());
  }
  return win::ExceptionDisposition::ContinueSearch;
}

EXTERN_C void __cxx_destroy_exception(catch_info& ci) noexcept {
  if (ci.throw_info_if_owner && ci.throw_info_if_owner->destroy_exc_obj) {
    const auto destructor{__ImageBase +
                          ci.throw_info_if_owner->destroy_exc_obj};
    destructor(ci.exception_object_or_link);
  }
}

EXTERN_C win::ExceptionDisposition __cxx_call_catch_frame_handler(
    win::exception_record* exception_record,
    byte* frame_ptr,
    win::x64_cpu_context*,
    void* dispatcher_ctx) {
  if (exception_record != &win::exc_record_cookie) {
    verify_seh(exception_record->code, exception_record->address,
               exception_record->flags.value());
    return win::ExceptionDisposition::ContinueSearch;
  }

  auto* ctx{static_cast<dispatcher_context*>(dispatcher_ctx)};
  auto* frame{reinterpret_cast<catch_frame*>(frame_ptr)};

  auto& ci{ctx->throw_frame->catch_info};

  if (ctx->cookie == &rethrow_probe_cookie) {
    if (!frame->catch_info.exception_object_or_link) {
      set_termination_context({BugCheckReason::CorruptedExceptionHandler,
                               reinterpret_cast<bugcheck_arg_t>(ctx->cookie),
                               reinterpret_cast<bugcheck_arg_t>(frame)});
      terminate();
    }

    if (frame->catch_info.throw_info_if_owner)
      ci.exception_object_or_link = &frame->catch_info;
    else
      ci.exception_object_or_link = frame->catch_info.exception_object_or_link;
  } else if (ctx->cookie == &unwind_cookie) {
    if (!ci.exception_object_or_link ||
        ci.exception_object_or_link == &frame->catch_info) {
      ci.exception_object_or_link = frame->catch_info.exception_object_or_link;
      ci.throw_info_if_owner = frame->catch_info.throw_info_if_owner;
    } else {
      __cxx_destroy_exception(frame->catch_info);
    }
    ci.primary_frame_ptr = frame->catch_info.primary_frame_ptr;
    ci.unwind_context = frame->catch_info.unwind_context;
  } else {
    return win::ExceptionDisposition::ContinueSearch;
  }

  return win::ExceptionDisposition::CxxHandler;
}

frame_handler get_frame_handler(const byte* image_base,
                                const unwind_info& info,
                                size_t unwind_node_count) noexcept {
  /*
   * Immediately after the array of unwind entries in memory is RVAs to the
   * exception handler and handler-specific data emitted by the compiler
   *
   * union {
   *      uint32_t exception_handler;
   *      uint32_t function_entry;
   * };
   * uint32_t exception_data[];
   *
   * Type of exception handler  Type of the associated data
   *   __C_specific_handler        Scope table
   *   __GSHandlerCheck            GS data
   *   __GSHandlerCheck_SEH        Scope table + GS data
   *   __CxxFrameHandler3          RVA to FuncInfo
   *   __CxxFrameHandler4          RVA to FuncInfo
   *   __GSHandlerCheck_EH         RVA to FuncInfo + GS data
   *
   * Also see
   * https://yurichev.com/mirrors/RE/Recon-2012-Skochinsky-Compiler-Internals.pdf
   */
  using handler_rva_t = relative_virtual_address<win::x64_frame_handler_t>;

  const auto* exception_handler_and_data{
      reinterpret_cast<const byte*>(info.data + unwind_node_count)};

  const auto* handler_rva{
      reinterpret_cast<const handler_rva_t*>(exception_handler_and_data)};
  const auto* data{exception_handler_and_data + sizeof(handler_rva_t)};

  return {image_base + *handler_rva, data};
}

static void prepare_for_non_cxx_handler(dispatcher_context& ctx,
                                        frame_walk_context& cpu_ctx,
                                        machine_frame& mach) noexcept {
  cpu_ctx.dummy_rsp = mach.rsp;
  cpu_ctx.dummy_rip = mach.rip;

  ctx.last_instruction = mach.rip;
  ctx.history_table = nullptr;

  /*
   * We do not yet support the continuation of unwinding if an exception was
   * thrown in the SEH __finally block (this means that the destructor threw the
   * exception)
   */
  ctx.scope_index = 0;
}

const unwind_info* execute_handler(dispatcher_context& ctx,
                                   frame_walk_context& cpu_ctx,
                                   machine_frame& mach) noexcept {
  const auto& pdata{*ctx.pdata};
  const byte* image_base{pdata.image_base()};

  ctx.fn = pdata.find_function_entry(mach.rip);
  const unwind_info* unwind_info{image_base + ctx.fn->unwind_info};

  constexpr auto handler_mask{flag_set{HandlerInfo::Exception} |
                              flag_set{HandlerInfo::Unwind}};
  if (const auto flags = flag_set<HandlerInfo>{unwind_info->flags};
      flags & handler_mask) {
    // The number of active slots is always odd
    const auto unwind_slots =
        (static_cast<size_t>(unwind_info->code_count) + 1ull) & ~1ull;

    auto [handler, extra_data]{
        get_frame_handler(image_base, *unwind_info, unwind_slots)};
    crt_assert_with_msg(handler,
                        "C++ exception must have an associated handler");
    ctx.extra_data = extra_data;

    auto* frame_ptr{reinterpret_cast<byte*>(
        unwind_info->frame_reg ? cpu_ctx.gp(unwind_info->frame_reg)
                               : mach.rsp)};

    prepare_for_non_cxx_handler(ctx, cpu_ctx, mach);

    [[maybe_unused]] auto exc_action{
        handler(&win::exc_record_cookie, frame_ptr,
                reinterpret_cast<win::x64_cpu_context*>(&cpu_ctx), &ctx)};
  }
  return unwind_info;
}

void probe_for_exception(const frame_walk_pdata& pdata,
                         throw_frame& frame) noexcept {
  auto ctx{make_context(&rethrow_probe_cookie, frame, pdata)};
  auto& [_, probe_ctx, probe_mach, ci]{frame};

  for (;;) {
    const unwind_info* unwind_info =
        execute_handler(ctx, probe_ctx, probe_mach);
    if (ci.exception_object_or_link) {
      break;
    }
    pdata.unwind(*unwind_info, probe_ctx, probe_mach);
  }
}

EXTERN_C const byte* __cxx_dispatch_exception(void* exception_object,
                                              const throw_info* throw_info,
                                              throw_frame& frame) noexcept {
  const auto pdata{frame_walk_pdata::for_this_image()};
  auto ctx{make_context(&unwind_cookie, frame, pdata)};

  auto& [_, cpu_ctx, mach, ci]{frame};

  ci.exception_object_or_link = exception_object;
  ci.throw_info_if_owner = throw_info;
  ci.primary_frame_ptr = nullptr;

  for (;;) {
    const auto* unwind_info{execute_handler(ctx, cpu_ctx, mach)};
    if (ctx.handler) {
      return ctx.handler;
    }
    pdata.unwind(*unwind_info, cpu_ctx, mach);
  }
}
}  // namespace ktl::crt::exc_engine::x64