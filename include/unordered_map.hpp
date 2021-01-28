#pragma once
#include <unordered_container_impl.hpp>

namespace ktl {

template <class Key,
          class Ty,
          class Hash = hash<Key>,
          class KeyEqual = equal_to<Key>,
          class BasicBytesAlloc = basic_paged_allocator<void>,
          size_t MaxLoadFactor100 = 80>
using unordered_flat_map = un::details::
    Table<true, Key, Ty, Hash, KeyEqual, BasicBytesAlloc, MaxLoadFactor100>;

template <class Key,
          class Ty,
          class Hash = hash<Key>,
          class KeyEqual = equal_to<Key>,
          class BasicBytesAlloc = basic_non_paged_allocator<void>,
          size_t MaxLoadFactor100 = 80>
using unordered_flat_map_non_paged = un::details::
    Table<true, Key, Ty, Hash, KeyEqual, BasicBytesAlloc, MaxLoadFactor100>;

template <class Key,
          class Ty,
          class Hash = hash<Key>,
          class KeyEqual = equal_to<Key>,
          class BasicBytesAlloc = basic_paged_allocator<void>,
          size_t MaxLoadFactor100 = 80>
using unordered_node_map = un::details::
    Table<false, Key, Ty, Hash, KeyEqual, BasicBytesAlloc, MaxLoadFactor100>;

template <class Key,
          class Ty,
          class Hash = hash<Key>,
          class KeyEqual = equal_to<Key>,
          class BasicBytesAlloc = basic_non_paged_allocator<void>,
          size_t MaxLoadFactor100 = 80>
using unordered_node_map_non_paged = un::details::
    Table<false, Key, Ty, Hash, KeyEqual, BasicBytesAlloc, MaxLoadFactor100>;

template <class Key,
          class Ty,
          class Hash = hash<Key>,
          class KeyEqual = equal_to<Key>,
          class BasicBytesAlloc = basic_paged_allocator<void>,
          size_t MaxLoadFactor100 = 80>
using unordered_map = un::details::Table<
    sizeof(pair<Key, Ty>) <= sizeof(size_t) * 6 &&
        is_nothrow_move_constructible<pair<Key, Ty>>::value &&
        is_nothrow_move_assignable<pair<Key, Ty>>::value,
    Key,
    Ty,
    Hash,
    KeyEqual,
    BasicBytesAlloc,
    MaxLoadFactor100>;

template <class Key,
          class Ty,
          class Hash = hash<Key>,
          class KeyEqual = equal_to<Key>,
          class BasicBytesAlloc = basic_non_paged_allocator<void>,
          size_t MaxLoadFactor100 = 80>
using unordered_map_non_paged = un::details::Table<
    sizeof(pair<Key, Ty>) <= sizeof(size_t) * 6 &&
        is_nothrow_move_constructible<pair<Key, Ty>>::value &&
        is_nothrow_move_assignable<pair<Key, Ty>>::value,
    Key,
    Ty,
    Hash,
    KeyEqual,
    BasicBytesAlloc,
    MaxLoadFactor100>;

}  // namespace ktl
