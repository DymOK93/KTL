#pragma once
#include <flag_set.hpp>
#include <member_ptr.h>
#include <rva.hpp>

namespace ktl::crt::exc_engine::x64 {

struct gs_handler_data {
  static constexpr uint32_t EHANDLER = 1;
  static constexpr uint32_t UHANDLER = 2;
  static constexpr uint32_t HAS_ALIGNMENT = 4;
  static constexpr uint32_t COOKIE_OFFSET_MASK = ~7u;

  uint32_t cookie_offset;
  int32_t aligned_base_offset;
  int32_t alignment;
};

struct handler_entry {
  uint32_t try_rva_low;
  uint32_t try_rva_high;
  union {
    relative_virtual_address<int32_t(void* ptrs, uint64_t frame_ptr)> filter;
    relative_virtual_address<void(bool, uint64_t frame_ptr)> unwinder;
  };
  uint32_t target_rva;
};

struct handler_data {
  uint32_t entry_count;
  handler_entry entries[1];
};

enum class CatchableProperty : uint32_t {
  IsSimpleType = 0x01,
  ByReferenceOnly = 0x02,
  HasVirtualBase = 0x04,
  IsWinRTHandle = 0x08,
  IsBadAlloc = 0x10,
};

struct catchable_type {
  flag_set<CatchableProperty> properties;
  relative_virtual_address<type_info const> desc;
  member_ptr offset;
  uint32_t size;
  relative_virtual_address<byte const> copy_fn;
};

struct catchable_type_list {
  uint32_t count;
  relative_virtual_address<const catchable_type> types[1];
};

enum class ThrowFlag : uint32_t {
  IsConst = 0x01,
  IsVolatile = 0x02,
  IsUnaligned = 0x04,
  IsPure = 0x08,
  IsWinRT = 0x10,
};

struct throw_info {
  flag_set<ThrowFlag> attributes;
  relative_virtual_address<void __fastcall(void*)> destroy_exc_obj;
  relative_virtual_address<int(...)> compat_fn;
  relative_virtual_address<const catchable_type_list> catchables;
};

enum class CatchFlag : uint32_t {
  IsConst = 1,
  IsVolatile = 2,
  IsUnaligned = 4,
  IsReference = 8,
  IsResumable = 0x10,
  IsEllipsis = 0x40,
  IsBadAllocCompat = 0x80,
  IsComplusEh = 0x80000000,
};

struct eh_node_catch {
  byte* primary_frame_ptr;
};

struct catch_handler {
  flag_set<CatchFlag> adjectives;
  relative_virtual_address<const type_info> type_desc;
  relative_virtual_address<void> catch_object_offset;
  relative_virtual_address<const byte> handler;
  relative_virtual_address<const eh_node_catch> node_offset;
};

struct try_block {
  /* 0x00 */ int32_t try_low;
  /* 0x04 */ int32_t try_high;
  /* 0x08 */ int32_t catch_high;
  /* 0x0c */ int32_t catch_count;
  /* 0x10 */ relative_virtual_address<const catch_handler> catch_handlers;
};

struct eh_region {
  relative_virtual_address<const byte> first_ip;
  int32_t state;
};

ALIGN(8) struct eh_node {
  int32_t state;
  int32_t reserved;  // unused now
};

struct unwind_graph_edge {
  int32_t next;
  relative_virtual_address<const byte> cleanup_handler;
};

enum class EhFlag : uint32_t {
  CompiledWithEhs = 1,
  IsNoexcept = 4,
};

struct function_eh_info {
  /* 0x00 */ uint32_t magic;  // MSVC's magic number
  /* 0x04 */ uint32_t state_count;
  /* 0x08 */ relative_virtual_address<const unwind_graph_edge> unwind_graph;
  /* 0x0c */ int32_t try_block_count;
  /* 0x10 */ relative_virtual_address<const try_block> try_blocks;
  /* 0x14 */ uint32_t region_count;
  /* 0x18 */ relative_virtual_address<const eh_region> regions;
  /* 0x1c */ relative_virtual_address<eh_node> eh_node_offset;

  // `magic_num >= 0x19930521`
  /* 0x20 */ uint32_t es_types;

  // `magic_num >= 0x19930522`
  /* 0x24 */ flag_set<EhFlag> eh_flags;
};

struct eh_handler_data {
  relative_virtual_address<const function_eh_info> eh_info;
};

}  // namespace ktl::crt::exc_engine::x64

#pragma once
