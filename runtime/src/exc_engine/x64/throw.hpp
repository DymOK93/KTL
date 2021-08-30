/**
 * Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 * Copyright (c) 2019-2021 Martin Vejnár (avakar)
 */
#pragma once

#include <catch.hpp>
#include <cpu_context.hpp>
#include <exception_info.hpp>
#include <rva.hpp>
#include <symbol.hpp>

namespace ktl::crt::exc_engine::x64 {
// Marked offsets are used in nt!__GSHandlerCheck
struct dispatcher_context {
  symbol* cookie;
  /*0x8*/ const byte* image_base;
  /*0x16*/ const function* fn;
  const frame_walk_pdata* pdata;
  throw_frame* throw_frame;
  void* padding;
  const byte* handler;
  /*0x56*/ const void* extra_data;
};

dispatcher_context make_context(symbol* cookie_,
                                throw_frame& frame_,
                                const frame_walk_pdata& pdata_) noexcept;
}  // namespace ktl::crt::exc_engine::x64

#pragma once
