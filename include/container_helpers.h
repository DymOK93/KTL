#pragma once
#include <ktlexcept.hpp>

namespace ktl::cont::details {
template <class SizeType>
static void throw_if_index_greater_than_size(SizeType index, SizeType size) {
  throw_exception_if<out_of_range>(index > size, "pos is out of range");
}

template <class ValueTy, class SizeType>
static decltype(auto) at_index_verified(ValueTy* data,
                                        SizeType idx,
                                        SizeType size) {
  throw_exception_if_not<out_of_range>(idx < size, "index is out of range");
  return data[idx];
}

DEFINE_HAS_NESTED_TYPE(value_type)
DEFINE_HAS_NESTED_TYPE(size_type)
}  // namespace ktl::cont::details