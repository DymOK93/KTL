#pragma once
#include <basic_types.h>
#include <smart_pointer.hpp>
#include <string.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

namespace ktl {
inline size_t hash_bytes(const void* ptr, size_t len) noexcept {
  static constexpr uint64_t m = UINT64_C(0xc6a4a7935bd1e995);
  static constexpr uint64_t seed = UINT64_C(0xe17a1465);
  static constexpr unsigned int r = 47;

  auto const* const data64 = static_cast<uint64_t const*>(ptr);
  uint64_t h = seed ^ (len * m);

  size_t const n_blocks = len / 8;
  for (size_t i = 0; i < n_blocks; ++i) {
    auto k = unaligned_load<uint64_t>(data64 + i);

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  auto const* const data8 = reinterpret_cast<uint8_t const*>(data64 + n_blocks);
  switch (len & 7U) {
    case 7:
      h ^= static_cast<uint64_t>(data8[6]) << 48U;
    case 6:
      h ^= static_cast<uint64_t>(data8[5]) << 40U;
    case 5:
      h ^= static_cast<uint64_t>(data8[4]) << 32U;
    case 4:
      h ^= static_cast<uint64_t>(data8[3]) << 24U;
    case 3:
      h ^= static_cast<uint64_t>(data8[2]) << 16U;
    case 2:
      h ^= static_cast<uint64_t>(data8[1]) << 8U;
    case 1:
      h ^= static_cast<uint64_t>(data8[0]);
      h *= m;
    default:
      break;
  }

  h ^= h >> r;
  h *= m;
  h ^= h >> r;
  return static_cast<size_t>(h);
}

inline size_t hash_int(uint64_t x) noexcept {
  // inspired by lemire's strongly universal hashing
  // https://lemire.me/blog/2018/08/15/fast-strongly-universal-64-bit-hashing-everywhere/
  //
  // Instead of shifts, we use rotations so we don't lose any bits.
  //
  // Added a final multiplcation with a constant for more mixing. It is most
  // important that the lower bits are well mixed.
  auto h1 = x * UINT64_C(0xA24BAED4963EE407);
  auto h2 = rotr(x, 32U) * UINT64_C(0x9FB21C651E98DF25);
  auto h = rotr(h1 + h2, 32U);
  return static_cast<size_t>(h);
}

template <class Ty, enable_if_t<is_trivial_v<Ty>, int> = 0>
size_t hash_array(const Ty* data, size_t obj_count) noexcept {
  return hash_bytes(data, sizeof(Ty) * obj_count);
}

template <class Ty, typename Enable = void>
struct hash;

template <size_t BufferSize,
          class NativeStrTy,
          template <typename... CharT>
          class Traits,
          class Alloc>
struct hash<basic_winnt_string<BufferSize, NativeStrTy, Traits, Alloc>> {
  size_t operator()(
      const basic_winnt_string<BufferSize, NativeStrTy, Traits, Alloc>& str)
      const noexcept {
   return hash_array(str.data(), str.size());
  }
};

template <typename NativeStrTy, template <typename... CharT> class Traits>
struct hash<basic_winnt_string_view<NativeStrTy, Traits>> {
  size_t operator()(
      basic_winnt_string_view<NativeStrTy, Traits> sv) const noexcept {
    return hash_array(sv.data(), sv.size());
  }
};

template <class Ty>
struct hash<Ty*> {
  size_t operator()(Ty* ptr) const noexcept {
    return hash_int(reinterpret_cast<size_t>(ptr));
  }
};

template <class Ty, class Dx>
struct hash<unique_ptr<Ty, Dx>> {
  size_t operator()(const unique_ptr<Ty, Dx>& ptr) const noexcept {
    return hash_int(reinterpret_cast<size_t>(ptr.get()));
  }
};

template <class Ty>
struct hash<shared_ptr<Ty>> {
  size_t operator()(const shared_ptr<Ty>& ptr) const noexcept {
    return hash_int(reinterpret_cast<size_t>(ptr.get()));
  }
};

template <typename Enum>
struct hash<Enum, typename enable_if_t<is_enum_v<Enum>>> {
  size_t operator()(Enum e) const noexcept {
    using underlying_t = underlying_type_t<Enum>;
    return hash<underlying_t>{}(static_cast<underlying_t>(e));
  }
};

#define HASH_INT(Ty)                                  \
  template <>                                         \
  struct hash<Ty> {                                   \
    size_t operator()(const Ty& obj) const noexcept { \
      return hash_int(static_cast<uint64_t>(obj));    \
    }                                                 \
  }

// see https://en.cppreference.com/w/cpp/utility/hash
HASH_INT(bool);
HASH_INT(char);
HASH_INT(signed char);
HASH_INT(unsigned char);
HASH_INT(char16_t);
HASH_INT(char32_t);
HASH_INT(wchar_t);
HASH_INT(short);
HASH_INT(unsigned short);
HASH_INT(int);
HASH_INT(unsigned int);
HASH_INT(long);
HASH_INT(long long);
HASH_INT(unsigned long);
HASH_INT(unsigned long long);
}  // namespace ktl