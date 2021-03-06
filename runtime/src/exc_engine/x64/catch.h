﻿/**
 * Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 * Copyright (c) 2019-2021 Martin Vejnár (avakar)
 * */
#pragma once
#include <cpu_context.h>
#include <exception_info.h>
#include <symbol.hpp>

namespace ktl::crt::exc_engine::x64 {
using copy_ctor_t = void(void* self, void* other);
using copy_ctor_virtual_base_t = void(void* self,
                                      void* other,
                                      int is_most_derived);

inline symbol unwind_cookie{};
inline symbol rethrow_probe_cookie{};

struct catch_info {
  const byte* continuation_address[2];
  byte* primary_frame_ptr;
  void* exception_object_or_link;
  const throw_info* throw_info_if_owner;
  uint64_t unwind_context{0};

  void* get_exception_object() const noexcept;
  const throw_info* get_throw_info() const noexcept;
};

struct throw_frame {
  uint64_t red_zone[4];

  frame_walk_context ctx;
  machine_frame mach;
  catch_info catch_info;
};

struct catch_frame {
  uint64_t red_zone[4];

  machine_frame mach;
  catch_info catch_info;
};

void probe_for_exception(const frame_walk_pdata& pdata,
                         throw_frame& frame) noexcept;

bool process_catch_block(
    const byte* image_base,
    flag_set<CatchFlag> adjectives,
    const type_info* match_type,
    void* catch_var,
    void* exception_object,
    const throw_info&
        throw_info) noexcept;  // Согласно Стандарту, конструктор копирования
                               // исключения не имеет права бросить исключение
}  // namespace ktl::crt::exc_engine::x64