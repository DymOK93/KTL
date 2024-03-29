#include <bugcheck.hpp>
#include <cpu_context.hpp>
#include <symbol.hpp>

#define BREAK_IF_FALSE(cond) \
  if (!(cond))               \
    break

EXTERN_C ktl::crt::exc_engine::symbol __ImageBase;

namespace ktl::crt::exc_engine::x64 {
namespace pe {
template <typename Ty>
struct dir_header {
  /*0x00*/ relative_virtual_address<Ty> relative_virtual_address;
  /*0x04*/ uint32_t size;
  /*0x08*/
};

struct extended_header {
  /*0x00*/ uint32_t magic;

  /*0x04*/ uint16_t machine;
  /*0x06*/ uint8_t _06[0x12];

  /*0x18*/ uint16_t opt_magic;
  /*0x1a*/ uint8_t _20[0x36];
  /*0x50*/ uint32_t image_size;
  /*0x54*/ uint32_t headers_size;
  /*0x58*/ uint8_t _58[0x2c];
  /*0x84*/ uint32_t directory_count;

  /*0x88*/ dir_header<void> export_table;
  /*0x90*/ dir_header<void> import_table;
  /*0x98*/ dir_header<void> resource_table;
  /*0xa0*/ dir_header<function> exception_table;
};

struct dos_header {
  /*0x00*/ uint16_t magic;
  /*0x02*/ uint8_t _02[0x3a];
  /*0x3c*/ relative_virtual_address<extended_header> image_header;
  /*0x40*/
};
}  // namespace pe

frame_walk_pdata frame_walk_pdata::for_this_image() noexcept {
  return frame_walk_pdata{__ImageBase};
}

const byte* frame_walk_pdata::image_base() const noexcept {
  return m_image_base;
}

bool frame_walk_pdata::contains_address(const byte* addr) const noexcept {
  return m_image_base <= addr && addr - m_image_base < m_image_size;
}

const function* frame_walk_pdata::find_function_entry(
    const byte* addr) const noexcept {
  if (!contains_address(addr)) {
    set_termination_context({BugCheckReason::NoMatchingExceptionHandler,
                             reinterpret_cast<bugcheck_arg_t>(addr)});
    terminate();
  }

  const auto pc_rva{make_rva(addr, m_image_base)};
  uint32_t left_bound{0};
  uint32_t right_bound = m_function_count;

  while (left_bound < right_bound) {
    const uint32_t idx{left_bound + (right_bound - left_bound) / 2};
    if (const auto* fn_ptr = m_functions + idx; pc_rva < fn_ptr->begin) {
      right_bound = idx;
    } else if (fn_ptr->end <= pc_rva) {
      left_bound = idx + 1;
    } else {
      return fn_ptr;
    }
  }
  return nullptr;
}

frame_walk_pdata::frame_walk_pdata(const byte* image_base) noexcept
    : m_image_base(image_base) {
  const auto* dos_hdr = reinterpret_cast<const pe::dos_header*>(image_base);

  do {
    BREAK_IF_FALSE(dos_hdr->magic == 0x5a4d);
    const pe::extended_header* pe_hdr{image_base + dos_hdr->image_header};

    BREAK_IF_FALSE(pe_hdr->magic == 0x4550);
    BREAK_IF_FALSE(pe_hdr->machine == 0x8664);
    BREAK_IF_FALSE(pe_hdr->opt_magic == 0x20b);
    BREAK_IF_FALSE(pe_hdr->headers_size >=
                   dos_hdr->image_header.value() + sizeof(pe::extended_header));
    BREAK_IF_FALSE(pe_hdr->image_size >= pe_hdr->headers_size);

    BREAK_IF_FALSE(pe_hdr->directory_count >= 4);
    BREAK_IF_FALSE(pe_hdr->exception_table.size % sizeof(function) == 0);

    m_functions = image_base + pe_hdr->exception_table.relative_virtual_address;
    m_function_count = pe_hdr->exception_table.size / sizeof(function);
    m_image_size = pe_hdr->image_size;

    return;

  } while (false);

  set_termination_context({BugCheckReason::CorruptedPeHeader});
  terminate();
}

uint64_t& frame_walk_context::gp(uint8_t idx) noexcept {
  constexpr int8_t conv[16]{
      -1, -1, -1, 0, -1, 1, 2, 3, -1, -1, -1, -1, 4, 5, 6, 7,
  };
  const int8_t offs = conv[idx];
  if (offs < 0) {
    set_termination_context({BugCheckReason::CorruptedMachineState, offs});
    terminate();
  }
  return (&rbx)[offs];
};

static xmm_register& get_xmm(frame_walk_context& ctx, uint8_t idx) noexcept {
  if (idx < 6 || idx >= 16) {
    set_termination_context({BugCheckReason::CorruptedMachineState, idx});
    terminate();
  }
  return (&ctx.xmm6)[idx - 6];
}

template <class Ty>
Ty get_unwind_data_as(const unwind_info& unwind_info, uint32_t idx) noexcept {
  const auto* data{reinterpret_cast<const byte*>(unwind_info.data + idx)};
  const auto value_ptr{reinterpret_cast<const Ty*>(data)};
  return *value_ptr;
}

void frame_walk_pdata::unwind(const unwind_info& unwind_info,
                              frame_walk_context& ctx,
                              machine_frame& mach) noexcept {
  bool rip_updated = false;
  for (uint32_t idx = 0; idx < unwind_info.code_count; ++idx) {
    switch (const auto& entry = unwind_info.entries[idx]; entry.code) {
      case UnwindCode::PushNonVolatileReg:
        ctx.gp(entry.info) = *reinterpret_cast<const uint64_t*>(mach.rsp);
        mach.rsp += 8;
        break;

      case UnwindCode::AllocLarge:
        if (!entry.info) {
          mach.rsp += unwind_info.data[++idx] * 8;
        } else {
          mach.rsp += unwind_info.data[idx + 1];
          idx += 2;
        }
        break;

      case UnwindCode::AllocSmall:
        mach.rsp += entry.info * 8 + 8;
        break;

      case UnwindCode::SetFramePointer:
        mach.rsp =
            ctx.gp(unwind_info.frame_reg) - unwind_info.frame_reg_disp * 16;
        break;

      case UnwindCode::SaveNonVolatileReg:
        ctx.gp(entry.info) = *reinterpret_cast<const uint64_t*>(
            mach.rsp + unwind_info.data[++idx] * 8);
        break;

      case UnwindCode::SaveFarNonVolatileReg:
        ctx.gp(entry.info) = *reinterpret_cast<const uint64_t*>(
            mach.rsp + get_unwind_data_as<uint32_t>(unwind_info, +idx));
        idx += 2;
        break;

      case UnwindCode::Epilog:
        idx += 1;
        break;

      case UnwindCode::Reserved07:
        idx += 2;
        break;

      case UnwindCode::SaveXmm128:
        get_xmm(ctx, entry.info) = *reinterpret_cast<const xmm_register*>(
            mach.rsp + unwind_info.data[++idx] * 16);
        break;

      case UnwindCode::SaveFarXmm128:
        get_xmm(ctx, entry.info) = *reinterpret_cast<const xmm_register*>(
            mach.rsp + unwind_info.data[idx + 1]);
        idx += 2;
        break;

      case UnwindCode::PushMachineFrame:
        mach.rsp += static_cast<uint64_t>(entry.info) * 8;
        mach.rip = *reinterpret_cast<const byte**>(mach.rsp);
        mach.rsp = *reinterpret_cast<const uint64_t*>(mach.rsp + 24);
        rip_updated = true;
        break;
    }
  }

  if (!rip_updated) {
    mach.rip = *reinterpret_cast<const byte**>(mach.rsp);
    mach.rsp += 8;
  }
}
}  // namespace ktl::crt::exc_engine::x64