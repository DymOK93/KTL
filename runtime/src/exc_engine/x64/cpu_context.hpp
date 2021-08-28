#pragma once
#include <member_ptr.hpp>
#include <flag_set.hpp>
#include <rva.hpp>

namespace ktl::crt::exc_engine::x64 {

enum class UnwindCode : uint8_t {
  PushNonVolatileReg = 0,     // 1
  AllocLarge = 1,             // 2-3
  AllocSmall = 2,             // 1
  SetFramePointer = 3,        // 1
  SaveNonVolatileReg = 4,     // 2
  SaveFarNonVolatileReg = 5,  // 3
  Epilog = 6,                 // 2
  Reserved07 = 7,             // 3 _07
  SaveXmm128 = 8,             // 2
  SaveFarXmm128 = 9,          // 3
  PushMachineFrame = 10,      // 1
};

struct unwind_entry {
  uint8_t prolog_offset;
  UnwindCode code : 4;
  uint8_t info : 4;
};

struct unwind_info {
  uint8_t version : 3;
  uint8_t flags : 5;
  uint8_t prolog_size;
  uint8_t unwind_code_count;
  uint8_t frame_reg : 4;
  uint8_t frame_reg_disp : 4;
  union {
    unwind_entry entries[1];
    uint16_t data[1];
  };
};

struct function {
  /* 0x00 */ relative_virtual_address<const byte> begin;
  /* 0x04 */ relative_virtual_address<const byte> end;
  /* 0x08 */ relative_virtual_address<const unwind_info> unwind_info;
};

/*struct frame_info_t
{
        relative_virtual_address<void const> function;
        relative_virtual_address<win32_frame_handler_t> exception_routine;
        relative_virtual_address<void const> extra_data;
        relative_virtual_address<void const> rip;
        uint64_t frame_ptr;
};*/

ALIGN(16) struct xmm_register { unsigned char data[16]; };

struct frame_walk_context {
  xmm_register xmm6;
  xmm_register xmm7;
  xmm_register xmm8;
  xmm_register xmm9;
  xmm_register xmm10;
  xmm_register xmm11;
  xmm_register xmm12;
  xmm_register xmm13;
  xmm_register xmm14;
  xmm_register xmm15;

  uint64_t rbx;
  uint64_t rbp;
  uint64_t rsi;
  uint64_t rdi;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;

  uint64_t& gp(uint8_t idx) noexcept;
};

struct machine_frame {
  const byte* rip;
  uint64_t cs;
  uint64_t eflags;
  uint64_t rsp;
  uint64_t ss;
};

class frame_walk_pdata {
 public:
  explicit frame_walk_pdata(const byte* image_base) noexcept;

  static frame_walk_pdata for_this_image() noexcept;

  const byte* image_base() const noexcept;
  bool contains_address(const byte* addr) const noexcept;
  const function* find_function_entry(const byte* addr) const noexcept;

  void unwind(unwind_info const& unwind_info,
              frame_walk_context& ctx,
              machine_frame& mach) const noexcept;

 private:
  const byte* m_image_base;
  const function* m_functions;
  uint32_t m_function_count;
  uint32_t m_image_size;
};

}  // namespace ktl::crt::exc_engine::x64

#pragma once
