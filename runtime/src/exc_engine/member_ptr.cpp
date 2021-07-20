#include <member_ptr.h>

namespace ktl::crt::exc_engine {
member_ptr::member_ptr(offset_t vbase_offset,
                       offset_t vbtable_ptr_offset,
                       offset_t member_offset) noexcept
    : m_vbase_offset{vbase_offset},
      m_vbtable_ptr_offset{vbtable_ptr_offset},
      m_member_offset{member_offset} {}

uintptr_t member_ptr::apply(uintptr_t obj) const noexcept {
  if (m_vbtable_ptr_offset) {
    uintptr_t vbtable_ptr = obj + m_vbtable_ptr_offset;
    uintptr_t vbtable = *reinterpret_cast<const uintptr_t*>(vbtable_ptr);
    obj = vbtable_ptr +
          *reinterpret_cast<const offset_t*>(vbtable + m_vbase_offset);
  }
  return obj + m_member_offset;
}
}  // namespace ktl::crt::exc_engine
