#pragma once
#include <exception.h>

namespace ktl::cont::details {
template <class SizeType>
static void throw_if_index_greater_than_size(SizeType index, SizeType size) {
  throw_exception_if<out_of_range>(index > size, L"pos is out of range",
                                   constexpr_message_tag{});
}

template <class ValueTy, class SizeType>
static decltype(auto) at_index_verified(ValueTy* data,
                                        SizeType idx,
                                        SizeType size) {
  throw_exception_if_not<out_of_range>(idx < size, L"index is out of range",
                                       constexpr_message_tag{});
  return data[idx];
}
}  // namespace ktl::cont::details