#pragma once
#include <../basic_types.hpp>

namespace ktl::crt::exc_engine {
using uintptr_t =
    int64_t;  // Определяем здесь во избежание конфликта с typedef'ом в vadef.h
using offset_t = uint32_t;

class member_ptr {
 public:
  member_ptr(offset_t vbase_offset,
             offset_t vbtable_ptr_offset,
             offset_t member_offset) noexcept;

  uintptr_t apply(uintptr_t obj) const noexcept;

 private:
  offset_t m_vbase_offset;
  offset_t m_vbtable_ptr_offset;
  offset_t m_member_offset;
};

}  // namespace ktl::crt::exc_engine