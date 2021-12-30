/**
 * Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 * Copyright (c) 2019-2021 Martin Vejnár (avakar)
 */
#pragma once
#include "catch.hpp"
#include "cpu_context.hpp"

#include <symbol.hpp>

namespace ktl::crt::exc_engine::x64 {
// Marked offsets are used by the nt!__GSHandlerCheck and nt!__C_speficic_handler
struct dispatcher_context {
  /*0x0*/ const byte* last_instruction;
  /*0x8*/ const byte* image_base;
  /*0x10*/ const function* fn;
  const frame_walk_pdata* pdata;
  throw_frame* throw_frame;
  void* padding;
  const byte* handler;
  /*0x38*/ const void* extra_data;
  void* history_table;
  /*0x48*/ uint32_t scope_index;
  symbol* cookie;
};

dispatcher_context make_context(symbol* cookie,
                                throw_frame& frame,
                                const frame_walk_pdata& pdata) noexcept;
}  // namespace ktl::crt::exc_engine::x64

#pragma once
