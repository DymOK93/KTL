#include <bugcheck.hpp>
#include <catch.hpp>
#include <seh.hpp>
#include <throw.hpp>

EXTERN_C ktl::crt::exc_engine::symbol __ImageBase;

namespace ktl::crt::exc_engine::x64 {
EXTERN_C void __cxx_call_catch_handler() noexcept;
EXTERN_C win::x64_frame_handler_t __cxx_seh_frame_handler;
EXTERN_C win::x64_frame_handler_t __cxx_call_catch_frame_handler;

EXTERN_C const ktl::byte* __cxx_dispatch_exception(void* exception_object,
                                                   throw_info const* throw_info,
                                                   throw_frame& frame) noexcept;

EXTERN_C void __cxx_destroy_exception(catch_info& ci) noexcept;

static bool process_catch_block_unchecked(const byte* image_base,
                                          flag_set<CatchFlag> adjectives,
                                          const type_info* match_type,
                                          void* catch_var,
                                          void* exception_object,
                                          const catchable_type_list*) noexcept;

static const unwind_info* execute_handler(dispatcher_context& ctx,
                                          frame_walk_context& cpu_ctx,
                                          machine_frame& mach) noexcept;

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

void probe_for_exception(const frame_walk_pdata& pdata,
                         throw_frame& frame) noexcept {
  auto ctx{make_context(&rethrow_probe_cookie, frame, pdata)};

  auto& probe_ctx{frame.ctx};
  auto& probe_mach = frame.mach;
  auto& catch_info = frame.catch_info;

  for (;;) {
    const unwind_info* unwind_info =
        execute_handler(ctx, probe_ctx, probe_mach);
    if (catch_info.exception_object_or_link) {
      break;
    }
    pdata.unwind(*unwind_info, probe_ctx, probe_mach);
  }
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

  // Вернёт false, если флаг IsConst, IsVolatile или IsUnaligned есть в наборе
  // throw, но отсутствует у catch
  if (throw_info.attributes
          .bit_intersection<ThrowFlag::IsConst, ThrowFlag::IsVolatile,
                            ThrowFlag::IsUnaligned>() &
      adjectives.bit_negation()) {  // Приоритет у оператора ~ выше, чем у &
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

EXTERN_C void __cxx_destroy_exception(catch_info& ci) noexcept {
  if (ci.throw_info_if_owner && ci.throw_info_if_owner->destroy_exc_obj) {
    auto* dtor{__ImageBase + ci.throw_info_if_owner->destroy_exc_obj};
    dtor(ci.exception_object_or_link);
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
    const type_info* catchable_type_desc = image_base + catchable->desc;
    if (catchable_type_desc == match_type) {
      if (!catchable->properties.has_any_of(
              CatchableProperty::ByReferenceOnly) ||
          adjectives.has_any_of(CatchFlag::IsReference)) {
        if (adjectives.has_any_of(CatchFlag::IsReference)) {
          auto* catch_var_holder{reinterpret_cast<void**>(catch_var)};
          *catch_var_holder = exception_object;
        } else {
          if (catchable->properties.has_any_of(
                  CatchableProperty::IsSimpleType)) {
            memcpy(catch_var, exception_object, catchable->size);
          } else if (!catchable->copy_fn) {
            const auto addr{catchable->offset.apply(
                reinterpret_cast<uintptr_t>(exception_object))};
            memcpy(catch_var, reinterpret_cast<void*>(addr), catchable->size);
          } else if (catchable->properties.has_any_of(
                         CatchableProperty::HasVirtualBase)) {
            auto* copy_ctor{reinterpret_cast<copy_ctor_virtual_base_t*>(
                image_base + catchable->copy_fn)};
            copy_ctor(catch_var, exception_object, 1);
          } else {
            auto* copy_ctor{reinterpret_cast<copy_ctor_t*>(image_base +
                                                           catchable->copy_fn)};
            copy_ctor(catch_var, exception_object);
          }
        }
        return true;
      }
    }
  }
  return false;
}

static const unwind_info* execute_handler(dispatcher_context& ctx,
                                          frame_walk_context& cpu_ctx,
                                          machine_frame& mach) noexcept {
  const auto& pdata{*ctx.pdata};
  const byte* image_base{pdata.image_base()};

  ctx.fn = pdata.find_function_entry(mach.rip);
  const unwind_info* unwind_info{image_base + ctx.fn->unwind_info};

  /*
   * UNW_FLAG_EHANDLER 0x1
   * UNW_FLAG_UHANDLER 0x2
   * See
   * https://docs.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-rtlvirtualunwind
   */
  constexpr auto handler_mask{flag_set{HandlerInfo::Exception} |
                              flag_set{HandlerInfo::Exception}};
  if (const auto flags = flag_set<HandlerInfo>{unwind_info->flags};
      flags & handler_mask) {
    constexpr auto mask{
        ~static_cast<size_t>(1)};  // The number of active slots is always odd
    const auto unwind_slots =
        (static_cast<size_t>(unwind_info->code_count) + 1ull) & mask;
    auto* frame_handler =
        image_base +
        *reinterpret_cast<
            const relative_virtual_address<win::x64_frame_handler_t>*>(
            unwind_info->data + unwind_slots);

    crt_assert(frame_handler);

    // Dummy fields
    cpu_ctx.dummy_rsp = mach.rsp;
    cpu_ctx.dummy_rip = mach.rip;
    // ctx.cookie = (symbol*)mach.rip;
    ctx.history_table = nullptr;
    ctx.scope_index = 0;

    ctx.extra_data = &unwind_info->data[unwind_slots + 2];  // Why 2?
    byte* frame_ptr = reinterpret_cast<byte*>(
        unwind_info->frame_reg ? cpu_ctx.gp(unwind_info->frame_reg) : mach.rsp);

    [[maybe_unused]] auto exc_action{
        frame_handler(&win::exc_record_cookie, frame_ptr,
                      reinterpret_cast<win::x64_cpu_context*>(&cpu_ctx), &ctx)};

    // MSVC добавляет __GSHandlerCheck() в начало таблицы исключений
    // Поскольку он всегда возвращает win::ExceptionDisposition::ContinueSearch
    // в случае успешной проверки, отслеживать exc_action ==
    // win::ExceptionDisposition::CxxHandler мы не можем
    // Однако SEH-исключение всё равно не пролетит мимо:
    // т.к. win::exception_record* != nullptr, в __CxxFrameHandler4 сработает
    // проверка и система упадёт в BSOD
  }
  return unwind_info;
}
}  // namespace ktl::crt::exc_engine::x64