#pragma once
#include <basic_types.hpp>
#include <hash.hpp>
#include <hash_table_impl.hpp>

namespace ktl {
template <class Key,
          class Hash = hash<Key>,
          class KeyEqual = equal_to<Key>,
          class BytesAllocator = basic_paged_allocator<byte>,
          size_t MaxLoadFactor100 = 80>
using unordered_flat_set = un::details::
    Table<true, Key, void, Hash, KeyEqual, BytesAllocator, MaxLoadFactor100>;

template <class Key,
          class Hash = hash<Key>,
          class KeyEqual = equal_to<Key>,
          class BytesAllocator = basic_non_paged_allocator<byte>,
          size_t MaxLoadFactor100 = 80>
using unordered_flat_set_non_paged = un::details::
    Table<true, Key, void, Hash, KeyEqual, BytesAllocator, MaxLoadFactor100>;

template <class Key,
          class Hash = hash<Key>,
          class KeyEqual = equal_to<Key>,
          class BytesAllocator = basic_paged_allocator<byte>,
          size_t MaxLoadFactor100 = 80>
using unordered_node_set = un::details::
    Table<false, Key, void, Hash, KeyEqual, BytesAllocator, MaxLoadFactor100>;

template <class Key,
          class Hash = hash<Key>,
          class KeyEqual = equal_to<Key>,
          class BytesAllocator = basic_non_paged_allocator<byte>,
          size_t MaxLoadFactor100 = 80>
using unordered_node_set_non_paged = un::details::
    Table<false, Key, void, Hash, KeyEqual, BytesAllocator, MaxLoadFactor100>;

template <class Key,
          class Hash = hash<Key>,
          class KeyEqual = equal_to<Key>,
          class BytesAllocator = basic_paged_allocator<byte>,
          size_t MaxLoadFactor100 = 80>
using unordered_set =
    un::details::Table<sizeof(Key) <= sizeof(size_t) * 6 &&
                           is_nothrow_move_constructible<Key>::value &&
                           is_nothrow_move_assignable<Key>::value,
                       Key,
                       void,
                       Hash,
                       KeyEqual,
                       BytesAllocator,
                       MaxLoadFactor100>;

template <class Key,
          class Hash = hash<Key>,
          class KeyEqual = equal_to<Key>,
          class BytesAllocator = basic_non_paged_allocator<byte>,
          size_t MaxLoadFactor100 = 80>
using unordered_set_non_paged =
    un::details::Table<sizeof(Key) <= sizeof(size_t) * 6 &&
                           is_nothrow_move_constructible<Key>::value &&
                           is_nothrow_move_assignable<Key>::value,
                       Key,
                       void,
                       Hash,
                       KeyEqual,
                       BytesAllocator,
                       MaxLoadFactor100>;

}  // namespace ktl
