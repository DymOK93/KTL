#include <bugcheck.hpp>
#include <seh.hpp>
#include <throw.hpp>

namespace ktl::crt::exc_engine::x64 {
namespace fh4 {
struct gs_check_data {
  uint32_t _00;
};

struct gs4_data {
  relative_virtual_address<uint8_t> func_info;
  gs_check_data gs_check_data;
};

enum class Attributes : uint8_t {
  IsCatchFunclet = 0x01,
  HasMultipleFunclets = 0x02,
  Bbt = 0x04,  // Flags set by Basic Block Transformations
  HasUnwindMap = 0x08,
  HasTryBlockMap = 0x10,
  Ehs = 0x20,
  IsNoexcept = 0x40,
};

struct info {
  flag_set<Attributes> flags;
  uint32_t bbt_flags;
  relative_virtual_address<const uint8_t> unwind_graph;
  relative_virtual_address<const uint8_t> try_blocks;
  relative_virtual_address<const uint8_t> regions;
  relative_virtual_address<byte*> primary_frame_ptr;
};

enum class CatchBlockFlag : uint8_t {
  HasTypeFlags = 0x01,
  HasTypeInfo = 0x02,
  HasCatchVar = 0x04,
  ImageRva = 0x08,
  ContinuationAddrCount = 0x30,
};

struct unwind_edge {
  enum class Type {
    Trivial = 0,
    ObjectOffset = 1,
    ObjectPtrOffset = 2,
    Function = 3,
  };

  uint32_t target_offset;
  Type type;
  relative_virtual_address<void()> destroy_fn;
  relative_virtual_address<void> object;

  static unwind_edge read(const uint8_t** p) noexcept;
  static void skip(const uint8_t** p) noexcept;
};
}  // namespace fh4

static win::ExceptionDisposition frame_handler(
    win::exception_record*,
    byte* frame_ptr,
    win::x64_cpu_context*,
    dispatcher_context* dispatcher_context) noexcept;

static uint32_t read_unsigned(const uint8_t** data) noexcept;
static int32_t read_int(const uint8_t** data) noexcept;

static void destroy_objects(
    const byte* image_base,
    relative_virtual_address<const uint8_t> unwind_graph_rva,
    byte* frame_ptr,
    int32_t initial_state,
    int32_t final_state) noexcept;

static void load_exception_info(fh4::info& eh_info,
                                const uint8_t* data,
                                const byte* image_base,
                                const function& fn) noexcept;

static int32_t lookup_region(const fh4::info* eh_info,
                             const byte* image_base,
                             relative_virtual_address<const byte> fn,
                             const byte* control_pc) noexcept;

template <typename Ty = void>
static relative_virtual_address<Ty> read_rva(const uint8_t** data) noexcept;

EXTERN_C win::ExceptionDisposition __CxxFrameHandler4(
    win::exception_record* exception_record,
    byte* frame_ptr,
    win::x64_cpu_context* cpu_ctx,
    dispatcher_context* dispatcher_ctx) noexcept {
  if (exception_record != &win::exc_record_cookie) {
    verify_seh_in_cxx_handler(exception_record->code, exception_record->address,
                              exception_record->flags.value(),
                              dispatcher_ctx->fn->unwind_info.value(),
                              dispatcher_ctx->image_base);
    return win::ExceptionDisposition::ContinueSearch;
  }
  return frame_handler(exception_record, frame_ptr, cpu_ctx, dispatcher_ctx);
}

EXTERN_C win::ExceptionDisposition __GSHandlerCheck_EH4(
    win::exception_record* exception_record,
    byte* frame_ptr,
    [[maybe_unused]] win::x64_cpu_context* cpu_ctx,
    dispatcher_context* ctx) noexcept {
  // No cookie check :(
  // We assume that the compiler will use only __GSHandlerCheck_SEH for SEH
  // exceptions and therefore don't check exception_record
  return __CxxFrameHandler4(exception_record, frame_ptr, cpu_ctx, ctx);
}

static win::ExceptionDisposition frame_handler(
    win::exception_record*,
    byte* frame_ptr,
    win::x64_cpu_context*,
    dispatcher_context* dispatcher_context) noexcept {
  if (dispatcher_context->cookie == &rethrow_probe_cookie) {
    return win::ExceptionDisposition::CxxHandler;
  }
  if (dispatcher_context->cookie != &unwind_cookie) {
    return win::ExceptionDisposition::ContinueSearch;
  }

  const byte* image_base{dispatcher_context->pdata->image_base()};
  auto* throw_frame{dispatcher_context->throw_frame};
  auto& catch_info{throw_frame->catch_info};

  const auto* handler_data{
      static_cast<const fh4::gs4_data*>(dispatcher_context->extra_data)};
  const uint8_t* compressed_data{image_base + handler_data->func_info};

  fh4::info eh_info = {};
  load_exception_info(eh_info, compressed_data, image_base,
                      *dispatcher_context->fn);

  byte* primary_frame_ptr;
  int32_t initial_state;

  if (catch_info.primary_frame_ptr >= frame_ptr) {
    primary_frame_ptr = catch_info.primary_frame_ptr;
    initial_state = static_cast<int32_t>(catch_info.unwind_context);
  } else {
    initial_state =
        lookup_region(&eh_info, image_base, dispatcher_context->fn->begin,
                      throw_frame->mach.rip);
    if (eh_info.flags.has_any_of(fh4::Attributes::IsCatchFunclet))
      primary_frame_ptr = *(frame_ptr + eh_info.primary_frame_ptr);
    else
      primary_frame_ptr = frame_ptr;
  }

  int32_t target_state = -1;

  if (eh_info.try_blocks && initial_state >= 0) {
    const auto* throw_info{catch_info.get_throw_info()};

    const uint8_t* p{image_base + eh_info.try_blocks};
    uint32_t try_block_count = read_unsigned(&p);
    for (uint32_t try_block_idx = 0; try_block_idx != try_block_count;
         ++try_block_idx) {
      uint32_t try_low = read_unsigned(&p);
      uint32_t try_high = read_unsigned(&p);

      [[maybe_unused]] uint32_t catch_high{read_unsigned(&p)};

      auto handlers{read_rva<const uint8_t>(&p)};

      if (try_low > static_cast<uint32_t>(initial_state) ||
          static_cast<uint32_t>(initial_state) > try_high) {
        continue;
      }

      if (!throw_info) {
        probe_for_exception(*dispatcher_context->pdata, *throw_frame);
        throw_info = throw_frame->catch_info.get_throw_info();
      }

      const uint8_t* q = image_base + handlers;
      uint32_t handler_count = read_unsigned(&q);

      for (uint32_t handler_idx = 0; handler_idx != handler_count;
           ++handler_idx) {
        flag_set<fh4::CatchBlockFlag> handler_flags{*q++};

        flag_set<CatchFlag> type_flags =
            handler_flags.has_any_of(fh4::CatchBlockFlag::HasTypeFlags)
                ? flag_set<CatchFlag>(read_unsigned(&q))
                : flag_set<CatchFlag>{};

        relative_virtual_address<type_info const> type =
            handler_flags.has_any_of(fh4::CatchBlockFlag::HasTypeInfo)
                ? read_rva<type_info const>(&q)
                : 0;

        uint32_t continuation_addr_count =
            handler_flags.get<fh4::CatchBlockFlag::ContinuationAddrCount>();

        relative_virtual_address<void> catch_var =
            handler_flags.has_any_of(fh4::CatchBlockFlag::HasCatchVar)
                ? relative_virtual_address<void>{read_unsigned(&q)}
                : 0;

        relative_virtual_address<const byte> handler = read_rva<const byte>(&q);

        if (handler_flags.has_any_of(fh4::CatchBlockFlag::ImageRva)) {
          for (uint32_t k = 0; k != continuation_addr_count; ++k)
            catch_info.continuation_address[k] =
                image_base + read_rva<const byte>(&q);
        } else {
          const byte* fn = image_base + dispatcher_context->fn->begin;
          for (uint32_t k = 0; k != continuation_addr_count; ++k)
            catch_info.continuation_address[k] = fn + read_unsigned(&q);
        }

        if (process_catch_block(image_base, type_flags, image_base + type,
                                primary_frame_ptr + catch_var,
                                throw_frame->catch_info.get_exception_object(),
                                *throw_info)) {
          dispatcher_context->handler = image_base + handler;
          catch_info.primary_frame_ptr = primary_frame_ptr;
          target_state = try_low - 1;
          catch_info.unwind_context = target_state;
          break;
        }
      }

      if (dispatcher_context->handler) {
        break;
      }
    }
  }

  if (target_state < initial_state) {
    destroy_objects(image_base, eh_info.unwind_graph, frame_ptr, initial_state,
                    target_state);
  }
  return win::ExceptionDisposition::CxxHandler;
}

fh4::unwind_edge fh4::unwind_edge::read(const uint8_t** p) noexcept {
  unwind_edge result;

  uint32_t target_offset_and_type = read_unsigned(p);
  auto type{static_cast<Type>(target_offset_and_type & 3)};
  result.type = type;
  result.target_offset = target_offset_and_type >> 2;
  result.destroy_fn = type != Type::Trivial ? read_rva<void()>(p) : 0;
  result.object = relative_virtual_address<void>(
      type == Type::ObjectOffset || type == Type::ObjectPtrOffset
          ? read_unsigned(p)
          : 0);

  return result;
}

void fh4::unwind_edge::skip(const uint8_t** p) noexcept {
  (void)read(p);
}

static uint32_t read_unsigned(const uint8_t** data) noexcept {
  uint8_t enc_type{static_cast<uint8_t>((*data)[0] & 0xf)};

  static constexpr uint8_t lengths[] = {
      1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1, 5,
  };
  static constexpr uint8_t shifts[] = {
      0x19, 0x12, 0x19, 0x0b, 0x19, 0x12, 0x19, 0x04,
      0x19, 0x12, 0x19, 0x0b, 0x19, 0x12, 0x19, 0x00,
  };

  uint8_t length = lengths[enc_type];

  // XXX we're in UB land here
  uint32_t result{*reinterpret_cast<const uint32_t*>(*data + length - 4)};
  result >>= shifts[enc_type];

  *data += length;
  return result;
}

static int32_t read_int(const uint8_t** data) noexcept {
  // XXX alignment

  int32_t result{*reinterpret_cast<const int32_t*>(*data)};
  *data += 4;
  return result;
}

template <typename Ty>
static relative_virtual_address<Ty> read_rva(const uint8_t** data) noexcept {
  uint32_t offset = read_int(data);
  return relative_virtual_address<Ty>{offset};
}

static void load_exception_info(fh4::info& eh_info,
                                const uint8_t* data,
                                const byte* image_base,
                                const function& fn) noexcept {
  flag_set<fh4::Attributes> flags{
      *reinterpret_cast<const fh4::Attributes*>(data++)};  // ?
  eh_info.flags = flags;

  if (flags.has_any_of(fh4::Attributes::Bbt)) {
    eh_info.bbt_flags = read_unsigned(&data);
  }

  if (flags.has_any_of(fh4::Attributes::HasUnwindMap)) {
    eh_info.unwind_graph = read_rva<const uint8_t>(&data);
  }

  if (flags.has_any_of(fh4::Attributes::HasTryBlockMap)) {
    eh_info.try_blocks = read_rva<const uint8_t>(&data);
  }

  if (flags.has_any_of(fh4::Attributes::HasMultipleFunclets)) {
    const uint8_t* funclet_map{image_base + read_rva<const uint8_t>(&data)};

    uint32_t count = read_unsigned(&funclet_map);
    eh_info.regions = 0;

    for (uint32_t idx = 0; idx != count; ++idx) {
      relative_virtual_address<void const> fn_rva =
          read_rva<void const>(&funclet_map);
      relative_virtual_address<const uint8_t> regions =
          read_rva<const uint8_t>(&funclet_map);

      if (fn_rva.value() == fn.begin.value()) {
        eh_info.regions = regions;
        break;
      }
    }
  } else {
    eh_info.regions = read_rva<const uint8_t>(&data);
  }

  if (flags.has_any_of(fh4::Attributes::IsCatchFunclet))
    eh_info.primary_frame_ptr =
        relative_virtual_address<byte*>(read_unsigned(&data));
}

static int32_t lookup_region(const fh4::info* eh_info,
                             const byte* image_base,
                             relative_virtual_address<const byte> fn,
                             const byte* control_pc) noexcept {
  if (!eh_info->regions)
    return -1;

  relative_virtual_address pc = make_rva(control_pc, image_base + fn);
  const uint8_t* p = image_base + eh_info->regions;

  int32_t state = -1;
  const uint32_t region_count = read_unsigned(&p);
  relative_virtual_address<const byte> fn_rva = 0;
  for (uint32_t idx = 0; idx != region_count; ++idx) {
    fn_rva += read_unsigned(&p);
    if (pc < fn_rva)
      break;

    state = read_unsigned(&p) - 1;
  }

  return state;
}

void destroy_objects(const byte* image_base,
                     relative_virtual_address<const uint8_t> unwind_graph_rva,
                     byte* frame_ptr,
                     int32_t initial_state,
                     int32_t final_state) noexcept {
  const uint8_t* unwind_graph{image_base + unwind_graph_rva};
  uint32_t unwind_node_count = read_unsigned(&unwind_graph);

  if (initial_state < 0 ||
      static_cast<uint32_t>(initial_state) >= unwind_node_count) {
    set_termination_context({BugCheckReason::CorruptedEhUnwindData,
                             initial_state, unwind_node_count});
    terminate();
  }

  const uint8_t *current_edge{unwind_graph}, *last_edge{current_edge};
  for (int32_t idx = 0; idx != initial_state; ++idx) {
    fh4::unwind_edge::skip(&current_edge);
    if (idx == final_state) {
      last_edge = current_edge;
    }
  }
  if (initial_state == final_state) {
    last_edge = current_edge;
  }

  for (;;) {
    const uint8_t* unwind_entry{current_edge};

    uint32_t target_offset_and_type{read_unsigned(&unwind_entry)};
    uint32_t target_offset{target_offset_and_type >> 2};

    if (!target_offset) {
      set_termination_context({BugCheckReason::CorruptedEhUnwindData,
                               initial_state,
                               unwind_node_count,
                               reinterpret_cast<bugcheck_arg_t>(current_edge),
                               target_offset_and_type});
      terminate();
    }

    auto edge_type{
        static_cast<fh4::unwind_edge::Type>(target_offset_and_type & 3)};

    switch (edge_type) {
      case fh4::unwind_edge::Type::Trivial:
        break;
      case fh4::unwind_edge::Type::ObjectOffset: {
        auto destroy_fn{image_base + read_rva<void(byte*)>(&unwind_entry)};
        byte* obj{frame_ptr + read_unsigned(&unwind_entry)};
        destroy_fn(obj);
        break;
      }
      case fh4::unwind_edge::Type::ObjectPtrOffset: {
        auto destroy_fn{image_base + read_rva<void(byte*)>(&unwind_entry)};
        byte* obj{*reinterpret_cast<byte**>(frame_ptr +
                                            read_unsigned(&unwind_entry))};
        destroy_fn(obj);
        break;
      }
      case fh4::unwind_edge::Type::Function: {
        auto destroy_fn{image_base +
                        read_rva<void(void*, byte*)>(&unwind_entry)};
        destroy_fn(destroy_fn, frame_ptr);
        break;
      }
    }

    if (current_edge - last_edge < target_offset) {
      break;
    }
    current_edge -= target_offset;
  }
}
}  // namespace ktl::crt::exc_engine::x64
