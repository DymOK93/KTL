/**
 * Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 * Copyright (c) 2019-2021 Martin Vejnár (avakar)
 */
#pragma once

#include <catch.h>
#include <cpu_context.h>
#include <exception_info.h>
#include <rva.hpp>
#include <symbol.hpp>

namespace ktl::crt::exc_engine::x64 {
struct dispatcher_context {
  symbol* cookie;
  throw_frame* throw_frame;
  const frame_walk_pdata* pdata;
  const function* fn;
  const void* extra_data;
  const byte* handler;
};

}  // namespace ktl::crt::exc_engine::x64

#pragma once
