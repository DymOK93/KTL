//                 ______  _____                 ______                _________
//  ______________ ___  /_ ___(_)_______         ___  /_ ______ ______ ______  /
//  __  ___/_  __ \__  __ \__  / __  __ \        __  __ \_  __ \_  __ \_  __  /
//  _  /    / /_/ /_  /_/ /_  /  _  / / /        _  / / // /_/ // /_/ // /_/ /
//  /_/     \____/ /_.___/ /_/   /_/ /_/ ________/_/ /_/ \____/ \____/ \__,_/
//                                      _/_____/
//
// Fast & memory efficient hashtable based on robin hood hashing for
// C++11/14/17/20 https://github.com/martinus/robin-hood-hashing
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 Martin Ankerl <http://martin.ankerl.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
// see https://semver.org/
#define ROBIN_HOOD_VERSION_MAJOR 3  // for incompatible API changes
#define ROBIN_HOOD_VERSION_MINOR \
  9  // for adding functionality in a backwards-compatible manner
#define ROBIN_HOOD_VERSION_PATCH 1  // for backwards-compatible bug fixes

#include <basic_types.h>
#include <exception.h>
#include <algorithm.hpp>
#include <allocator.hpp>
#include <functional.hpp>
#include <hash.hpp>
#include <initializer_list.hpp>
#include <intrinsic.hpp>
#include <iterator.hpp>
#include <limits.hpp>
#include <tuple.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

#define COUNT_TRAILING_ZEROES(x)                                  \
  [](size_t mask) noexcept -> int {                               \
    unsigned long index;                                          \
    return BITSCANFORWARD(&index, mask) ? static_cast<int>(index) \
                                                                  \
                                        : BITNESS;                \
  }(x)

namespace ktl {
namespace un::details {
// Allocates bulks of memory for objects of type Ty. This deallocates the memory
// in the destructor, and keeps a linked list of the allocated memory around.
// Overhead per allocation is the size of a pointer.
template <class Ty,
          class BytesAllocator,
          size_t MinNumAllocs = 4,
          size_t MaxNumAllocs = 256>
class BulkPoolAllocator {
 public:
  using allocator_type = BytesAllocator;

 private:
  using AlBytesTraits = allocator_traits<allocator_type>;

 public:
  template <class BytesAlloc = allocator_type>
  BulkPoolAllocator(BytesAlloc&& alloc = BytesAlloc{}) noexcept(
      is_nothrow_constructible_v<allocator_type, BytesAlloc>)
      : m_bytes_alc{forward<BytesAlloc>(alloc)} {
    static_assert(AlBytesTraits::propagate_on_container_move_assignment::value,
                  "bytes allocator must be move assignable");
    static_assert(AlBytesTraits::propagate_on_container_swap::value,
                  "bytes allocator must be swappable");
  }

  // does not copy anything, just creates a new allocator.
  BulkPoolAllocator([[maybe_unused]] const BulkPoolAllocator& other) noexcept(
      is_nothrow_default_constructible_v<allocator_type>)
      : mHead(nullptr), mListForFree(nullptr) {}

  BulkPoolAllocator(BulkPoolAllocator&& other) noexcept(
      is_nothrow_move_constructible_v<allocator_type>)
      : m_bytes_alc{move(other.m_bytes_alc)},
        mHead{exchange(other.mHead, nullptr)},
        mListForFree{(other.mListForFree, nullptr)} {}

  BulkPoolAllocator& operator=(BulkPoolAllocator&& other) noexcept(
      is_nothrow_move_assignable_v<allocator_type>) {
    reset();
    m_bytes_alc = move(other.m_bytes_alc);
    mHead = exchange(other.mHead, nullptr);
    mListForFree = exchange(other.mListForFree, nullptr);
    return *this;
  }

  BulkPoolAllocator&
  // NOLINTNEXTLINE(bugprone-unhandled-self-assignment,cert-oop54-cpp)
  operator=(const BulkPoolAllocator& other) noexcept(
      is_nothrow_default_constructible_v<allocator_type>) {
    return *this;
  }

  ~BulkPoolAllocator() noexcept { reset(); }

  // Deallocates all allocated memory.
  void reset() noexcept {
    while (mListForFree) {
      Ty* tmp = *mListForFree;
      AlBytesTraits::deallocate_bytes(m_bytes_alc, mListForFree, ALIGNED_SIZE);
      mListForFree = reinterpret_cast_no_cast_align_warning<Ty**>(tmp);
    }
    mHead = nullptr;
  }

  // allocates, but does NOT initialize. Use in-place new constructor, e.g.
  //   Ty* obj = pool.allocate();
  //   ::new (static_cast<void*>(obj)) Ty();
  Ty* allocate() {
    Ty* tmp = mHead;
    if (!tmp) {
      tmp = performAllocation();
    }

    mHead = *reinterpret_cast_no_cast_align_warning<Ty**>(tmp);
    return tmp;
  }

  Ty* allocate_bytes(size_t bytes_count) {
    return reinterpret_cast<Ty*>(
        AlBytesTraits::allocate_bytes(m_bytes_alc, bytes_count));
  }

  // does not actually deallocate but puts it in store.
  // make sure you have already called the destructor! e.g. with
  //  obj->~Ty();
  //  pool.deallocate(obj);
  void deallocate(Ty* obj) noexcept {
    *reinterpret_cast_no_cast_align_warning<Ty**>(obj) = mHead;
    mHead = obj;
  }

  // deallocate
  // make sure you have already called the destructor! e.g. with
  //  obj->~Ty();
  void deallocate_bytes(void* ptr, size_t bytes_count) {
    AlBytesTraits::deallocate_bytes(m_bytes_alc, ptr, bytes_count);
  }

  // Adds an already allocated block of memory to the allocator. This allocator
  // is from now on responsible for freeing the data (with free()). If the
  // provided data is not large enough to make use of, it is immediately freed.
  // Otherwise it is reused and freed in the destructor.
  void addOrFree(void* ptr, size_t numBytes) noexcept {
    // calculate number of available elements in ptr
    if (numBytes < ALIGNMENT + ALIGNED_SIZE) {
      // not enough data for at least one element. Free and return.
      deallocate_bytes(ptr, numBytes);
    } else {
      add(ptr, numBytes);
    }
  }

  void swap(BulkPoolAllocator<Ty, BytesAllocator, MinNumAllocs, MaxNumAllocs>&
                other) noexcept(is_nothrow_swappable_v<allocator_type>) {
    using ktl::swap;
    swap(m_bytes_alc, other.m_bytes_alc);
    swap(mHead, other.mHead);
    swap(mListForFree, other.mListForFree);
  }

 private:
  // iterates the list of allocated memory to calculate how many to alloc next.
  // Recalculating this each time saves us a size_t member.
  // This ignores the fact that memory blocks might have been added manually
  // with addOrFree. In practice, this should not matter much.
  [[nodiscard]] size_t calcNumElementsToAlloc() const noexcept {
    auto tmp = mListForFree;
    size_t numAllocs = MinNumAllocs;

    while (numAllocs * 2 <= MaxNumAllocs && tmp) {
      auto x = reinterpret_cast<Ty***>(tmp);
      tmp = *x;
      numAllocs *= 2;
    }

    return numAllocs;
  }

  // WARNING: Underflow if numBytes < ALIGNMENT! This is guarded in addOrFree().
  void add(void* ptr, const size_t numBytes) noexcept {
    const size_t numElements = (numBytes - ALIGNMENT) / ALIGNED_SIZE;

    auto data = reinterpret_cast<Ty**>(ptr);

    // link free list
    auto x = reinterpret_cast<Ty***>(data);
    *x = mListForFree;
    mListForFree = data;

    // create linked list for newly allocated data
    auto* const headT = reinterpret_cast_no_cast_align_warning<Ty*>(
        reinterpret_cast<char*>(ptr) + ALIGNMENT);

    auto* const head = reinterpret_cast<char*>(headT);

    // Visual Studio compiler automatically unrolls this loop, which is pretty
    // cool
    for (size_t i = 0; i < numElements; ++i) {
      *reinterpret_cast_no_cast_align_warning<char**>(head + i * ALIGNED_SIZE) =
          head + (i + 1) * ALIGNED_SIZE;
    }

    // last one points to 0
    *reinterpret_cast_no_cast_align_warning<Ty**>(
        head + (numElements - 1) * ALIGNED_SIZE) = mHead;
    mHead = headT;
  }

  // Called when no memory is available (mHead == 0).
  // Don't inline this slow path.
  NOINLINE Ty* performAllocation() {
    size_t const numElementsToAlloc = calcNumElementsToAlloc();

    // alloc new memory: [prev |Ty, Ty, ... Ty]
    size_t const bytes = ALIGNMENT + ALIGNED_SIZE * numElementsToAlloc;
    add(allocate_bytes(bytes), bytes);
    return mHead;
  }

  static constexpr size_t ALIGNMENT =
      (max)(alignment_of<Ty>::value, alignment_of<Ty*>::value);

  static constexpr size_t ALIGNED_SIZE =
      ((sizeof(Ty) - 1) / ALIGNMENT + 1) * ALIGNMENT;

  static_assert(MinNumAllocs >= 1, "MinNumAllocs");
  static_assert(MaxNumAllocs >= MinNumAllocs, "MaxNumAllocs");
  static_assert(ALIGNED_SIZE >= sizeof(Ty*), "ALIGNED_SIZE");
  static_assert(0 == (ALIGNED_SIZE % sizeof(Ty*)), "ALIGNED_SIZE mod");
  static_assert(ALIGNMENT >= sizeof(Ty*), "ALIGNMENT");

 private:
  allocator_type m_bytes_alc;
  Ty* mHead{nullptr};
  Ty** mListForFree{nullptr};
};

template <class Ty,
          class BytesAllocator,
          size_t MinSize,
          size_t MaxSize,
          bool IsFlat>
struct NodeAllocator;

template <class Ty, class BytesAllocator, size_t MinSize, size_t MaxSize>
class NodeAllocator<Ty, BytesAllocator, MinSize, MaxSize, true> {
 public:
  using allocator_type = BytesAllocator;

 private:
  using AlBytesTraits = allocator_traits<allocator_type>;

 public:
  template <class BytesAlloc = allocator_type>
  NodeAllocator(BytesAlloc&& alloc = BytesAlloc{}) noexcept(
      is_nothrow_constructible_v<allocator_type, BytesAlloc>)
      : m_bytes_alc{forward<BytesAlloc>(alloc)} {
    static_assert(AlBytesTraits::propagate_on_container_move_assignment::value,
                  "bytes allocator must be move assignable");
    static_assert(AlBytesTraits::propagate_on_container_swap::value,
                  "bytes allocator must be swappable");
  }

  Ty* allocate_bytes(size_t bytes_count) {
    return reinterpret_cast<Ty*>(
        AlBytesTraits::allocate_bytes(m_bytes_alc, bytes_count));
  }

  void deallocate_bytes(void* ptr, size_t bytes_count) {
    AlBytesTraits::deallocate_bytes(m_bytes_alc, ptr, bytes_count);
  }

  // we are not using the data, so just free it.
  void addOrFree(void* ptr, [[maybe_unused]] size_t bytes_count) noexcept {
    deallocate_bytes(ptr, bytes_count);
  }

 private:
  allocator_type m_bytes_alc;
};

template <class Ty, class BytesAllocator, size_t MinSize, size_t MaxSize>
struct NodeAllocator<Ty, BytesAllocator, MinSize, MaxSize, false>
    : public BulkPoolAllocator<Ty, BytesAllocator, MinSize, MaxSize> {};

// dummy hash, unsed as mixer when robin_hood::hash is already used
template <typename Ty>
struct identity_hash {
  constexpr size_t operator()(const Ty& obj) const noexcept {
    return static_cast<size_t>(obj);
  }
};
}  // namespace un::details

struct is_transparent_tag {};

namespace un::details {
template <typename Ty>
struct void_type {
  using type = void;
};

template <typename Ty, typename = void>
struct has_is_transparent : public false_type {};

template <typename Ty>
struct has_is_transparent<Ty,
                          typename void_type<typename Ty::is_transparent>::type>
    : public true_type {};

// using wrapper classes for hash and key_equal prevents the diamond problem
// when the same type is used. see https://stackoverflow.com/a/28771920/48181
template <typename Ty>
struct WrapHash : public Ty {
  WrapHash() = default;
  explicit WrapHash(Ty const& o) noexcept(noexcept(Ty(declval<Ty const&>())))
      : Ty(o) {}
};

template <typename Ty>
struct WrapKeyEqual : public Ty {
  WrapKeyEqual() = default;
  explicit WrapKeyEqual(Ty const& o) noexcept(
      noexcept(Ty(declval<Ty const&>())))
      : Ty(o) {}
};

// A highly optimized hashmap implementation, using the Robin Hood algorithm.
//
// In most cases, this map should be usable as a drop-in replacement for
// unordered_map, but be about 2x faster in most cases and require much
// less allocations.
//
// This implementation uses the following memory layout:
//
// [Node, Node, ... Node | info, info, ... infoSentinel ]
//
// * Node: either a DataNode that directly has the pair<key, val> as
// member,
//   or a DataNode with a pointer to pair<key,val>. Which DataNode
//   representation to use depends on how fast the swap() operation is.
//   Heuristically, this is automatically choosen based on sizeof(). there are
//   always 2^n Nodes.
//
// * info: Each Node in the map has a corresponding info byte, so there are 2^n
// info bytes.
//   Each byte is initialized to 0, meaning the corresponding Node is empty. Set
//   to 1 means the corresponding node contains data. Set to 2 means the
//   corresponding Node is filled, but it actually belongs to the previous
//   position and was pushed out because that place is already taken.
//
// * infoSentinel: Sentinel byte set to 1, so that iterator's ++ can stop at
// end() without the
//   need for a idx variable.
//
// According to STL, order of templates has effect on throughput. That's why
// I've moved the boolean to the front.
// https://www.reddit.com/r/cpp/comments/ahp6iu/compile_time_binary_size_reductions_and_cs_future/eeguck4/
template <bool IsFlat,
          typename Key,
          typename Ty,
          typename Hash,
          typename KeyEqual,
          class BytesAllocator,
          size_t MaxLoadFactor100>
class Table
    : public WrapHash<Hash>,
      public WrapKeyEqual<KeyEqual>,
      NodeAllocator<typename conditional<
                        is_void<Ty>::value,
                        Key,
                        pair<typename conditional<IsFlat, Key, Key const>::type,
                             Ty>>::type,
                    BytesAllocator,
                    4,
                    16384,
                    IsFlat> {
 public:
  static constexpr bool is_flat = IsFlat;
  static constexpr bool is_map = !is_void<Ty>::value;
  static constexpr bool is_set = !is_map;
  static constexpr bool is_transparent =
      has_is_transparent<Hash>::value && has_is_transparent<KeyEqual>::value;

  using key_type = Key;
  using mapped_type = Ty;
  using value_type =
      typename conditional_t<is_set,
                             Key,
                             pair<conditional_t<is_flat, Key, const Key>, Ty>>;
  using size_type = size_t;
  using hasher = Hash;
  using key_equal = KeyEqual;
  using Self = Table<IsFlat,
                     key_type,
                     mapped_type,
                     hasher,
                     key_equal,
                     BytesAllocator,
                     MaxLoadFactor100>;

 private:
  static_assert(MaxLoadFactor100 > 10 && MaxLoadFactor100 < 100,
                "MaxLoadFactor100 needs to be >10 && < 100");

  using WHash = WrapHash<Hash>;
  using WKeyEqual = WrapKeyEqual<KeyEqual>;

  // configuration defaults

  // make sure we have 8 elements, needed to quickly rehash mInfo
  static constexpr size_t InitialNumElements = sizeof(uint64_t);
  static constexpr uint32_t InitialInfoNumBits = 5;
  static constexpr uint8_t InitialInfoInc = 1U << InitialInfoNumBits;
  static constexpr size_t InfoMask = InitialInfoInc - 1U;
  static constexpr uint8_t InitialInfoHashShift = 0;
  using DataPool = NodeAllocator<value_type, BytesAllocator, 4, 16384, IsFlat>;

  // type needs to be wider than uint8_t.
  using InfoType = uint32_t;

  // DataNode ////////////////////////////////////////////////////////

  // Primary template for the data node. We have special implementations for
  // small and big objects. For large objects it is assumed that swap() is
  // fairly slow, so we allocate these on the heap so swap merely swaps a
  // pointer.
  template <typename M, bool>
  class DataNode {};

  // Small: just allocate on the stack.
  template <typename M>
  class DataNode<M, true> final {
   public:
    template <typename... Args>
    explicit DataNode([[maybe_unused]] M& map, Args&&... args) noexcept(
        noexcept(value_type(forward<Args>(args)...)))
        : mData(forward<Args>(args)...) {}

    DataNode([[maybe_unused]] M& map, DataNode<M, true>&& n) noexcept(
        is_nothrow_move_constructible<value_type>::value)
        : mData(move(n.mData)) {}

    // doesn't do anything
    void destroy([[maybe_unused]] M& map) noexcept {}
    void destroyDoNotDeallocate() noexcept {}

    value_type const* operator->() const noexcept { return &mData; }
    value_type* operator->() noexcept { return &mData; }

    const value_type& operator*() const noexcept { return mData; }

    value_type& operator*() noexcept { return mData; }

    template <typename VT = value_type>
    [[nodiscard]] typename enable_if<is_map, typename VT::first_type&>::type
    getFirst() noexcept {
      return mData.first;
    }
    template <typename VT = value_type>
    [[nodiscard]] typename enable_if<is_set, VT&>::type getFirst() noexcept {
      return mData;
    }

    template <typename VT = value_type>
    [[nodiscard]]
    typename enable_if<is_map, typename VT::first_type const&>::type
    getFirst() const noexcept {
      return mData.first;
    }
    template <typename VT = value_type>
    [[nodiscard]] typename enable_if<is_set, VT const&>::type getFirst()
        const noexcept {
      return mData;
    }

    template <typename MT = mapped_type>
    [[nodiscard]] typename enable_if<is_map, MT&>::type getSecond() noexcept {
      return mData.second;
    }

    template <typename MT = mapped_type>
    [[nodiscard]] typename enable_if<is_set, MT const&>::type getSecond()
        const noexcept {
      return mData.second;
    }

    void swap(DataNode<M, true>& o) noexcept(
        noexcept(declval<value_type>().swap(declval<value_type>()))) {
      mData.swap(o.mData);
    }

   private:
    value_type mData;
  };

  // big object: allocate on heap.
  template <typename M>
  class DataNode<M, false> {
   public:
    template <typename... Args>
    explicit DataNode(M& map, Args&&... args) : mData(map.allocate()) {
      construct_at(mData, forward<Args>(args)...);
    }

    DataNode([[maybe_unused]] M& map, DataNode<M, false>&& n) noexcept
        : mData(move(n.mData)) {}

    void destroy(M& map) noexcept {
      // don't deallocate, just put it into list of datapool.
      destroy_at(mData);
      map.deallocate(mData);
    }

    void destroyDoNotDeallocate() noexcept { destroy_at(mData); }

    value_type const* operator->() const noexcept { return mData; }

    value_type* operator->() noexcept { return mData; }

    const value_type& operator*() const { return *mData; }

    value_type& operator*() { return *mData; }

    template <typename VT = value_type>
    [[nodiscard]] typename enable_if<is_map, typename VT::first_type&>::type
    getFirst() noexcept {
      return mData->first;
    }
    template <typename VT = value_type>
    [[nodiscard]] typename enable_if<is_set, VT&>::type getFirst() noexcept {
      return *mData;
    }

    template <typename VT = value_type>
    [[nodiscard]]
    typename enable_if<is_map, typename VT::first_type const&>::type
    getFirst() const noexcept {
      return mData->first;
    }
    template <typename VT = value_type>
    [[nodiscard]] typename enable_if<is_set, VT const&>::type getFirst()
        const noexcept {
      return *mData;
    }

    template <typename MT = mapped_type>
    [[nodiscard]] typename enable_if<is_map, MT&>::type getSecond() noexcept {
      return mData->second;
    }

    template <typename MT = mapped_type>
    [[nodiscard]] typename enable_if<is_map, MT const&>::type getSecond()
        const noexcept {
      return mData->second;
    }

    void swap(DataNode<M, false>& o) noexcept {
      using ktl::swap;
      swap(mData, o.mData);
    }

   private:
    value_type* mData;
  };

  using Node = DataNode<Self, IsFlat>;

  // helpers for doInsert: extract first entry (only const required)
  [[nodiscard]] key_type const& getFirstConst(Node const& n) const noexcept {
    return n.getFirst();
  }

  // in case we have void mapped_type, we are not using a pair, thus we just
  // route k through. No need to disable this because it's just not used if not
  // applicable.
  [[nodiscard]] key_type const& getFirstConst(
      key_type const& k) const noexcept {
    return k;
  }

  // in case we have non-void mapped_type, we have a standard pair
  template <typename Q = mapped_type>
  [[nodiscard]] typename enable_if<!is_void<Q>::value, key_type const&>::type
  getFirstConst(value_type const& vt) const noexcept {
    return vt.first;
  }

  // Cloner //////////////////////////////////////////////////////////

  template <typename M, bool UseMemcpy>
  struct Cloner;

  // fast path: Just copy data, without allocating anything.
  template <typename M>
  struct Cloner<M, true> {
    void operator()(M const& source, M& target) const {
      auto const* const src = reinterpret_cast<char const*>(source.mKeyVals);
      auto* tgt = reinterpret_cast<char*>(target.mKeyVals);
      auto const numElementsWithBuffer =
          target.calcNumElementsWithBuffer(target.mMask + 1);
      copy(src, src + target.calcNumBytesTotal(numElementsWithBuffer), tgt);
    }
  };

  template <typename M>
  struct Cloner<M, false> {
    void operator()(M const& s, M& t) const {
      auto const numElementsWithBuffer =
          t.calcNumElementsWithBuffer(t.mMask + 1);
      copy(s.mInfo, s.mInfo + t.calcNumBytesInfo(numElementsWithBuffer),
           t.mInfo);

      for (size_t i = 0; i < numElementsWithBuffer; ++i) {
        if (t.mInfo[i]) {
          construct_at(t.mKeyVals + i, t, *s.mKeyVals[i]);
        }
      }
    }
  };

  // Destroyer ///////////////////////////////////////////////////////

  template <typename M, bool IsFlatAndTrivial>
  struct Destroyer {};

  template <typename M>
  struct Destroyer<M, true> {
    void nodes(M& m) const noexcept { m.mNumElements = 0; }

    void nodesDoNotDeallocate(M& m) const noexcept { m.mNumElements = 0; }
  };

  template <typename M>
  struct Destroyer<M, false> {
    void nodes(M& m) const noexcept {
      m.mNumElements = 0;
      // clear also resets mInfo to 0, that's sometimes not necessary.
      auto const numElementsWithBuffer =
          m.calcNumElementsWithBuffer(m.mMask + 1);

      for (size_t idx = 0; idx < numElementsWithBuffer; ++idx) {
        if (0 != m.mInfo[idx]) {
          Node& n = m.mKeyVals[idx];
          n.destroy(m);
          destroy_at(addressof(n));
        }
      }
    }

    void nodesDoNotDeallocate(M& m) const noexcept {
      m.mNumElements = 0;
      // clear also resets mInfo to 0, that's sometimes not necessary.
      auto const numElementsWithBuffer =
          m.calcNumElementsWithBuffer(m.mMask + 1);
      for (size_t idx = 0; idx < numElementsWithBuffer; ++idx) {
        if (0 != m.mInfo[idx]) {
          Node& n = m.mKeyVals[idx];
          n.destroyDoNotDeallocate();
          destroy_at(addressof(n));
        }
      }
    }
  };

  // Iter ////////////////////////////////////////////////////////////

  struct fast_forward_tag {};

  // generic iterator for both const_iterator and iterator.
  template <bool IsConst>
  // NOLINTNEXTLINE(hicpp-special-member-functions,cppcoreguidelines-special-member-functions)
  class Iter {
   private:
    using NodePtr = typename conditional<IsConst, Node const*, Node*>::type;

   public:
    using difference_type = ptrdiff_t;
    using value_type = typename Self::value_type;
    using reference =
        typename conditional<IsConst, value_type const&, value_type&>::type;
    using pointer =
        typename conditional<IsConst, value_type const*, value_type*>::type;
    using iterator_category = forward_iterator_tag;

    // default constructed iterator can be compared to itself, but WON'Ty return
    // true when compared to end().
    Iter() = default;

    // Rule of zero: nothing specified. The conversion constructor is only
    // enabled for iterator to const_iterator, so it doesn't accidentally work
    // as a copy ctor.

    // Conversion constructor from iterator to const_iterator.
    template <bool OtherIsConst,
              typename = typename enable_if<IsConst && !OtherIsConst>::type>
    // NOLINTNEXTLINE(hicpp-explicit-conversions)
    Iter(Iter<OtherIsConst> const& other) noexcept
        : mKeyVals(other.mKeyVals), mInfo(other.mInfo) {}

    Iter(NodePtr valPtr, uint8_t const* infoPtr) noexcept
        : mKeyVals(valPtr), mInfo(infoPtr) {}

    Iter(NodePtr valPtr, uint8_t const* infoPtr, fast_forward_tag) noexcept
        : mKeyVals(valPtr), mInfo(infoPtr) {
      fastForward();
    }

    template <bool OtherIsConst,
              typename = typename enable_if<IsConst && !OtherIsConst>::type>
    Iter& operator=(Iter<OtherIsConst> const& other) noexcept {
      mKeyVals = other.mKeyVals;
      mInfo = other.mInfo;
      return *this;
    }

    // prefix increment. Undefined behavior if we are at end()!
    Iter& operator++() noexcept {
      mInfo++;
      mKeyVals++;
      fastForward();
      return *this;
    }

    Iter operator++(int) noexcept {
      Iter tmp = *this;
      ++(*this);
      return tmp;
    }

    reference operator*() const { return **mKeyVals; }

    pointer operator->() const { return &**mKeyVals; }

    template <bool O>
    bool operator==(Iter<O> const& o) const noexcept {
      return mKeyVals == o.mKeyVals;
    }

    template <bool O>
    bool operator!=(Iter<O> const& o) const noexcept {
      return mKeyVals != o.mKeyVals;
    }

   private:
    // fast forward to the next non-free info byte
    // I've tried a few variants that don't depend on intrinsics, but
    // unfortunately they are quite a bit slower than this one. So I've reverted
    // that change again. See map_benchmark.
    void fastForward() noexcept {
      size_t n = 0;
      while (0U == (n = unaligned_load<size_t>(mInfo))) {
        mInfo += sizeof(size_t);
        mKeyVals += sizeof(size_t);
      }
      auto inc = COUNT_TRAILING_ZEROES(n) / 8;
      mInfo += inc;
      mKeyVals += inc;
    }

    friend class Table<IsFlat,
                       key_type,
                       mapped_type,
                       hasher,
                       key_equal,
                       BytesAllocator,
                       MaxLoadFactor100>;
    NodePtr mKeyVals{nullptr};
    uint8_t const* mInfo{nullptr};
  };

  ////////////////////////////////////////////////////////////////////

  // highly performance relevant code.
  // Lower bits are used for indexing into the array (2^n size)
  // The upper 1-5 bits need to be a reasonable good hash, to save comparisons.
  template <typename HashKey>
  void keyToIdx(HashKey&& key, size_t* idx, InfoType* info) const {
    // for a user-specified hash that is *not* robin_hood::hash, apply
    // robin_hood::hash as an additional mixing step. This serves as a bad hash
    // prevention, if the given data is badly mixed.
    using Mix = typename conditional<is_same_v<hash<key_type>, hasher>,
                                     identity_hash<size_t>, hash<size_t>>::type;

    // the lower InitialInfoNumBits are reserved for info.
    auto h = Mix{}(WHash::operator()(key));
    *info = mInfoInc + static_cast<InfoType>((h & InfoMask) >> mInfoHashShift);
    *idx = (h >> InitialInfoNumBits) & mMask;
  }

  // forwards the index by one, wrapping around at the end
  void next(InfoType* info, size_t* idx) const noexcept {
    *idx = *idx + 1;
    *info += mInfoInc;
  }

  void nextWhileLess(InfoType* info, size_t* idx) const noexcept {
    // unrolling this by hand did not bring any speedups.
    while (*info < mInfo[*idx]) {
      next(info, idx);
    }
  }

  // Shift everything up by one element. Tries to move stuff around.
  void shiftUp(size_t startIdx, size_t const insertion_idx) noexcept(
      is_nothrow_move_assignable<Node>::value) {
    auto idx = startIdx;
    construct_at(mKeyVals + idx, move(mKeyVals[idx - 1]));
    while (--idx != insertion_idx) {
      mKeyVals[idx] = move(mKeyVals[idx - 1]);
    }

    idx = startIdx;
    while (idx != insertion_idx) {
      mInfo[idx] = static_cast<uint8_t>(mInfo[idx - 1] + mInfoInc);
      if (mInfo[idx] + mInfoInc > 0xFF) {
        mMaxNumElementsAllowed = 0;
      }
      --idx;
    }
  }

  void shiftDown(size_t idx) noexcept(is_nothrow_move_assignable<Node>::value) {
    // until we find one that is either empty or has zero offset.
    // TODO(martinus) we don't need to move everything, just the last one for
    // the same bucket.
    mKeyVals[idx].destroy(*this);

    // until we find one that is either empty or has zero offset.
    while (mInfo[idx + 1] >= 2 * mInfoInc) {
      mInfo[idx] = static_cast<uint8_t>(mInfo[idx + 1] - mInfoInc);
      mKeyVals[idx] = move(mKeyVals[idx + 1]);
      ++idx;
    }

    mInfo[idx] = 0;
    // don't destroy, we've moved it
    // mKeyVals[idx].destroy(*this);
    destroy_at(mKeyVals + idx);
  }

  // copy of find(), except that it returns iterator instead of const_iterator.
  template <typename Other>
  [[nodiscard]] size_t findIdx(Other const& key) const {
    size_t idx{};
    InfoType info{};
    keyToIdx(key, &idx, &info);

    do {
      // unrolling this twice gives a bit of a speedup. More unrolling did not
      // help.
      if (info == mInfo[idx] &&
          WKeyEqual::operator()(key, mKeyVals[idx].getFirst())) {
        return idx;
      }
      next(&info, &idx);
      if (info == mInfo[idx] &&
          WKeyEqual::operator()(key, mKeyVals[idx].getFirst())) {
        return idx;
      }
      next(&info, &idx);
    } while (info <= mInfo[idx]);

    // nothing found!
    return mMask == 0
               ? 0
               : static_cast<size_t>(distance(
                     mKeyVals,
                     reinterpret_cast_no_cast_align_warning<Node*>(mInfo)));
  }

  void cloneData(const Table& o) {
    Cloner<Table, IsFlat && is_trivially_copyable_v<Node>>()(o, *this);
  }

  // inserts a keyval that is guaranteed to be new, e.g. when the hashmap is
  // resized.
  // @return index where the element was created
  size_t insert_move(Node&& keyval) {
    // we don't retry, fail if overflowing
    // don't need to check max num elements
    if (0 == mMaxNumElementsAllowed && !try_increase_info()) {
      throwOverflowError();  // impossible to reach LCOV_EXCL_LINE
    }

    size_t idx{};
    InfoType info{};
    keyToIdx(keyval.getFirst(), &idx, &info);

    // skip forward. Use <= because we are certain that the element is not
    // there.
    while (info <= mInfo[idx]) {
      idx = idx + 1;
      info += mInfoInc;
    }

    // key not found, so we are now exactly where we want to insert it.
    auto const insertion_idx = idx;
    auto const insertion_info = static_cast<uint8_t>(info);
    if (insertion_info + mInfoInc > 0xFF) {
      mMaxNumElementsAllowed = 0;
    }

    // find an empty spot
    while (0 != mInfo[idx]) {
      next(&info, &idx);
    }

    auto& l = mKeyVals[insertion_idx];
    if (idx == insertion_idx) {
      construct_at(&l, move(keyval));
    } else {
      shiftUp(idx, insertion_idx);
      l = move(keyval);
    }

    // put at empty spot
    mInfo[insertion_idx] = insertion_info;

    ++mNumElements;
    return insertion_idx;
  }

 public:
  using iterator = Iter<false>;
  using const_iterator = Iter<true>;

  Table() noexcept(noexcept(Hash()) && noexcept(KeyEqual()))
      : WHash(), WKeyEqual(), DataPool() {}

  // Creates an empty hash map. Nothing is allocated yet, this happens at the
  // first insert. This tremendously speeds up ctor & dtor of a map that never
  // receives an element. The penalty is payed at the first insert, and not
  // before. Lookup of this empty map works because everybody points to
  // DummyInfoByte::b. parameter bucket_count is dictated by the standard, but
  // we can ignore it.
  explicit Table(
      [[maybe_unused]] size_t bucket_count,
      const Hash& h = Hash{},
      const KeyEqual& equal =
          KeyEqual{}) noexcept(noexcept(Hash(h)) && noexcept(KeyEqual(equal)))
      : WHash(h), WKeyEqual(equal), DataPool() {}

  template <class BytesAlloc>
  explicit Table(
      [[maybe_unused]] size_t bucket_count,
      const Hash& h = Hash{},
      const KeyEqual& equal = KeyEqual{},
      BytesAlloc&& alloc =
          BytesAlloc{}) noexcept(noexcept(Hash(h)) && noexcept(KeyEqual(equal)))
      : WHash(h), WKeyEqual(equal), DataPool(forward<BytesAlloc>(alloc)) {}

  template <typename Iter>
  Table(Iter first,
        Iter last,
        [[maybe_unused]] size_t bucket_count = 0,
        const Hash& h = Hash{},
        const KeyEqual& equal = KeyEqual{})
      : WHash(h), WKeyEqual(equal) {
    insert(first, last);
  }

  template <typename Iter, class BytesAlloc>
  Table(Iter first,
        Iter last,
        [[maybe_unused]] size_t bucket_count = 0,
        const Hash& h = Hash{},
        const KeyEqual& equal = KeyEqual{},
        BytesAlloc&& alloc = BytesAlloc{})
      : WHash(h), WKeyEqual(equal), NodeAllocator(forward<BytesAlloc>(alloc)) {
    insert(first, last);
  }

  // initializer_list hasn't been fully implemented yet
  /* Table(initializer_list<value_type> init_list,
         [[maybe_unused]] size_t bucket_count = 0,
         const Hash& h = Hash{},
         const KeyEqual& equal = KeyEqual{})
       : WHash(h), WKeyEqual(equal), NodeAllocator() {
     insert(init_list.begin(), init_list.end());
   }

   template <class BytesAlloc>
   Table(initializer_list<value_type> init_list,
         [[maybe_unused]] size_t bucket_count = 0,
         const Hash& h = Hash{},
         const KeyEqual& equal = KeyEqual{},
         BytesAlloc&& alloc = BytesAlloc{})
       : WHash(h), WKeyEqual(equal), NodeAllocator(forward<BytesAlloc>(alloc)) {
     insert(init_list.begin(), init_list.end());
   }*/

  Table(Table&& o) noexcept
      : WHash(move(static_cast<WHash&>(o))),
        WKeyEqual(move(static_cast<WKeyEqual&>(o))),
        DataPool(move(static_cast<DataPool&>(o))) {
    if (o.mMask) {
      mKeyVals = move(o.mKeyVals);
      mInfo = move(o.mInfo);
      mNumElements = move(o.mNumElements);
      mMask = move(o.mMask);
      mMaxNumElementsAllowed = move(o.mMaxNumElementsAllowed);
      mInfoInc = move(o.mInfoInc);
      mInfoHashShift = move(o.mInfoHashShift);
      // set other's mask to 0 so its destructor won't do anything
      o.init();
    }
  }

  Table& operator=(Table&& o) noexcept {
    if (&o != this) {
      if (o.mMask) {
        // only move stuff if the other map actually has some data
        destroy();
        mKeyVals = move(o.mKeyVals);
        mInfo = move(o.mInfo);
        mNumElements = move(o.mNumElements);
        mMask = move(o.mMask);
        mMaxNumElementsAllowed = move(o.mMaxNumElementsAllowed);
        mInfoInc = move(o.mInfoInc);
        mInfoHashShift = move(o.mInfoHashShift);
        WHash::operator=(move(static_cast<WHash&>(o)));
        WKeyEqual::operator=(move(static_cast<WKeyEqual&>(o)));
        DataPool::operator=(move(static_cast<DataPool&>(o)));

        o.init();

      } else {
        // nothing in the other map => just clear us.
        clear();
      }
    }
    return *this;
  }

  Table(const Table& o)
      : WHash(static_cast<const WHash&>(o)),
        WKeyEqual(static_cast<const WKeyEqual&>(o)),
        DataPool(static_cast<const DataPool&>(o)) {
    if (!o.empty()) {
      // not empty: create an exact copy. it is also possible to just iterate
      // through all elements and insert them, but copying is probably faster.

      auto const numElementsWithBuffer = calcNumElementsWithBuffer(o.mMask + 1);
      auto const numBytesTotal = calcNumBytesTotal(numElementsWithBuffer);
      mKeyVals = reinterpret_cast<Node*>(allocate_bytes(numBytesTotal));
      // no need for calloc because clonData does memcpy
      mInfo = reinterpret_cast<uint8_t*>(mKeyVals + numElementsWithBuffer);
      mNumElements = o.mNumElements;
      mMask = o.mMask;
      mMaxNumElementsAllowed = o.mMaxNumElementsAllowed;
      mInfoInc = o.mInfoInc;
      mInfoHashShift = o.mInfoHashShift;
      cloneData(o);
    }
  }

  // Creates a copy of the given map. Copy constructor of each entry is used.
  // Not sure why clang-tidy thinks this doesn't handle self assignment, it does
  // NOLINTNEXTLINE(bugprone-unhandled-self-assignment,cert-oop54-cpp)
  Table& operator=(Table const& o) {
    if (&o == this) {
      // prevent assigning of itself
      return *this;
    }

    // we keep using the old allocator and not assign the new one, because we
    // want to keep the memory available. when it is the same size.
    if (o.empty()) {
      if (0 == mMask) {
        // nothing to do, we are empty too
        return *this;
      }

      // not empty: destroy what we have there
      // clear also resets mInfo to 0, that's sometimes not necessary.
      destroy();
      init();
      WHash::operator=(static_cast<const WHash&>(o));
      WKeyEqual::operator=(static_cast<const WKeyEqual&>(o));
      DataPool::operator=(static_cast<DataPool const&>(o));

      return *this;
    }

    // clean up old stuff
    Destroyer<Self, IsFlat && is_trivially_destructible<Node>::value>{}.nodes(
        *this);

    if (mMask != o.mMask) {
      // no luck: we don't have the same array size allocated, so we need to
      // realloc.
      if (0 != mMask) {
        // only deallocate if we actually have data!
        deallocate_bytes(mKeyVals, mNumElements * sizeof(Node));
      }

      auto const numElementsWithBuffer = calcNumElementsWithBuffer(o.mMask + 1);
      auto const numBytesTotal = calcNumBytesTotal(numElementsWithBuffer);
      mKeyVals = reinterpret_cast<Node*>(allocate_bytes(numBytesTotal));

      // no need for calloc here because cloneData performs a memcpy.
      mInfo = reinterpret_cast<uint8_t*>(mKeyVals + numElementsWithBuffer);
      // sentinel is set in cloneData
    }
    WHash::operator=(static_cast<const WHash&>(o));
    WKeyEqual::operator=(static_cast<const WKeyEqual&>(o));
    DataPool::operator=(static_cast<DataPool const&>(o));
    mNumElements = o.mNumElements;
    mMask = o.mMask;
    mMaxNumElementsAllowed = o.mMaxNumElementsAllowed;
    mInfoInc = o.mInfoInc;
    mInfoHashShift = o.mInfoHashShift;
    cloneData(o);

    return *this;
  }

  // Swaps everything between the two maps.
  void swap(Table& o) { swap(o, *this); }

  // Clears all data, without resizing.
  void clear() {
    if (empty()) {
      // don't do anything! also important because we don't want to write to
      // DummyInfoByte::b, even though we would just write 0 to it.
      return;
    }

    Destroyer<Self, IsFlat && is_trivially_destructible<Node>::value>{}.nodes(
        *this);

    auto const numElementsWithBuffer = calcNumElementsWithBuffer(mMask + 1);
    // clear everything, then set the sentinel again
    uint8_t const z = 0;
    // TODO: ktl::fill
    // fill(mInfo, mInfo + calcNumBytesInfo(numElementsWithBuffer), z);
    memset(mInfo, z, calcNumBytesInfo(numElementsWithBuffer));
    mInfo[numElementsWithBuffer] = 1;

    mInfoInc = InitialInfoInc;
    mInfoHashShift = InitialInfoHashShift;
  }

  // Destroys the map and all it's contents.
  ~Table() { destroy(); }

  // Checks if both tables contain the same entries. Order is irrelevant.
  bool operator==(const Table& other) const {
    if (other.size() != size()) {
      return false;
    }
    for (auto const& otherEntry : other) {
      if (!has(otherEntry)) {
        return false;
      }
    }

    return true;
  }

  bool operator!=(const Table& other) const { return !operator==(other); }

  template <typename Q = mapped_type>
  typename enable_if<!is_void<Q>::value, Q&>::type operator[](
      const key_type& key) {
    return doCreateByKey(key);
  }

  template <typename Q = mapped_type>
  typename enable_if<!is_void<Q>::value, Q&>::type operator[](key_type&& key) {
    return doCreateByKey(move(key));
  }

  template <typename Iter>
  void insert(Iter first, Iter last) {
    for (; first != last; ++first) {
      // value_type ctor needed because this might be called with pair's
      insert(value_type(*first));
    }
  }

  template <typename... Args>
  pair<iterator, bool> emplace(Args&&... args) {
    Node n{*this, forward<Args>(args)...};
    auto r = doInsert(move(n));
    if (!r.second) {
      // insertion not possible: destroy node
      // NOLINTNEXTLINE(bugprone-use-after-move)
      n.destroy(*this);
    }
    return r;
  }

  template <typename... Args>
  pair<iterator, bool> try_emplace(const key_type& key, Args&&... args) {
    return try_emplace_impl(key, forward<Args>(args)...);
  }

  template <typename... Args>
  pair<iterator, bool> try_emplace(key_type&& key, Args&&... args) {
    return try_emplace_impl(move(key), forward<Args>(args)...);
  }

  template <typename... Args>
  pair<iterator, bool> try_emplace(const_iterator hint,
                                   const key_type& key,
                                   Args&&... args) {
    (void)hint;
    return try_emplace_impl(key, forward<Args>(args)...);
  }

  template <typename... Args>
  pair<iterator, bool> try_emplace([[maybe_unused]] const_iterator hint,
                                   key_type&& key,
                                   Args&&... args) {
    (void)hint;
    return try_emplace_impl(move(key), forward<Args>(args)...);
  }

  template <typename Mapped>
  pair<iterator, bool> insert_or_assign(const key_type& key, Mapped&& obj) {
    return insert_or_assign_impl(key, forward<Mapped>(obj));
  }

  template <typename Mapped>
  pair<iterator, bool> insert_or_assign(key_type&& key, Mapped&& obj) {
    return insert_or_assign_impl(move(key), forward<Mapped>(obj));
  }

  template <typename Mapped>
  pair<iterator, bool> insert_or_assign([[maybe_unused]] const_iterator hint,
                                        const key_type& key,
                                        Mapped&& obj) {
    return insert_or_assign_impl(key, forward<Mapped>(obj));
  }

  template <typename Mapped>
  pair<iterator, bool> insert_or_assign([[maybe_unused]] const_iterator hint,
                                        key_type&& key,
                                        Mapped&& obj) {
    return insert_or_assign_impl(move(key), forward<Mapped>(obj));
  }

  pair<iterator, bool> insert(const value_type& keyval) {
    return doInsert(keyval);
  }

  pair<iterator, bool> insert(value_type&& keyval) {
    return doInsert(move(keyval));
  }

  // Returns 1 if key is found, 0 otherwise.
  size_t count(const key_type& key) const {  // NOLINT(modernize-use-nodiscard)
    auto kv = mKeyVals + findIdx(key);
    if (kv != reinterpret_cast_no_cast_align_warning<Node*>(mInfo)) {
      return 1;
    }
    return 0;
  }

  template <typename OtherKey, typename MySelf = Self>
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  typename enable_if<MySelf::is_transparent, size_t>::type count(
      const OtherKey& key) const {
    auto kv = mKeyVals + findIdx(key);
    if (kv != reinterpret_cast_no_cast_align_warning<Node*>(mInfo)) {
      return 1;
    }
    return 0;
  }

  bool contains(const key_type& key) const {  // NOLINT(modernize-use-nodiscard)
    return bool_cast(count(key));
  }

  template <typename OtherKey, typename Self_ = Self>
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  typename enable_if<Self_::is_transparent, bool>::type contains(
      const OtherKey& key) const {
    return bool_cast(count(key));
  }

  // Returns a reference to the value found for key.
  // Throws out_of_range if element cannot be found
  template <typename Q = mapped_type>
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  typename enable_if<!is_void<Q>::value, Q&>::type at(key_type const& key) {
    auto kv = mKeyVals + findIdx(key);
    if (kv == reinterpret_cast_no_cast_align_warning<Node*>(mInfo)) {
      throw_exception<out_of_range>(L"key not found");
    }
    return kv->getSecond();
  }

  // Returns a reference to the value found for key.
  // Throws out_of_range if element cannot be found
  template <typename Q = mapped_type>
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  typename enable_if<!is_void<Q>::value, Q const&>::type at(
      key_type const& key) const {
    auto kv = mKeyVals + findIdx(key);
    if (kv == reinterpret_cast_no_cast_align_warning<Node*>(mInfo)) {
      throw_exception<out_of_range>(L"key not found");
    }
    return kv->getSecond();
  }

  const_iterator find(
      const key_type& key) const {  // NOLINT(modernize-use-nodiscard)
    const size_t idx = findIdx(key);
    return const_iterator{mKeyVals + idx, mInfo + idx};
  }

  template <typename OtherKey>
  const_iterator find(const OtherKey& key,
                      is_transparent_tag /*unused*/) const {
    const size_t idx = findIdx(key);
    return const_iterator{mKeyVals + idx, mInfo + idx};
  }

  template <typename OtherKey, typename Self_ = Self>
  typename enable_if<Self_::is_transparent,  // NOLINT(modernize-use-nodiscard)
                     const_iterator>::type   // NOLINT(modernize-use-nodiscard)
  find(const OtherKey& key) const {          // NOLINT(modernize-use-nodiscard)
    const size_t idx = findIdx(key);
    return const_iterator{mKeyVals + idx, mInfo + idx};
  }

  iterator find(const key_type& key) {
    const size_t idx = findIdx(key);
    return iterator{mKeyVals + idx, mInfo + idx};
  }

  template <typename OtherKey>
  iterator find(const OtherKey& key, is_transparent_tag /*unused*/) {
    const size_t idx = findIdx(key);
    return iterator{mKeyVals + idx, mInfo + idx};
  }

  template <typename OtherKey, typename Self_ = Self>
  typename enable_if<Self_::is_transparent, iterator>::type find(
      const OtherKey& key) {
    const size_t idx = findIdx(key);
    return iterator{mKeyVals + idx, mInfo + idx};
  }

  iterator begin() {
    if (empty()) {
      return end();
    }
    return iterator(mKeyVals, mInfo, fast_forward_tag{});
  }
  const_iterator begin() const {  // NOLINT(modernize-use-nodiscard)
    return cbegin();
  }
  const_iterator cbegin() const {  // NOLINT(modernize-use-nodiscard)
    if (empty()) {
      return cend();
    }
    return const_iterator(mKeyVals, mInfo, fast_forward_tag{});
  }

  iterator end() {
    // no need to supply valid info pointer: end() must not be dereferenced, and
    // only node pointer is compared.
    return iterator{reinterpret_cast_no_cast_align_warning<Node*>(mInfo),
                    nullptr};
  }
  const_iterator end() const {  // NOLINT(modernize-use-nodiscard)
    return cend();
  }
  const_iterator cend() const {  // NOLINT(modernize-use-nodiscard)
    return const_iterator{reinterpret_cast_no_cast_align_warning<Node*>(mInfo),
                          nullptr};
  }

  iterator erase(const_iterator pos) {
    // its safe to perform const cast here
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return erase(iterator{const_cast<Node*>(pos.mKeyVals),
                          const_cast<uint8_t*>(pos.mInfo)});
  }

  // Erases element at pos, returns iterator to the next element.
  iterator erase(iterator pos) {
    // we assume that pos always points to a valid entry, and not end().
    auto const idx = static_cast<size_t>(pos.mKeyVals - mKeyVals);

    shiftDown(idx);
    --mNumElements;

    if (*pos.mInfo) {
      // we've backward shifted, return this again
      return pos;
    }

    // no backward shift, return next element
    return ++pos;
  }

  size_t erase(const key_type& key) {
    size_t idx{};
    InfoType info{};
    keyToIdx(key, &idx, &info);

    // check while info matches with the source idx
    do {
      if (info == mInfo[idx] &&
          WKeyEqual::operator()(key, mKeyVals[idx].getFirst())) {
        shiftDown(idx);
        --mNumElements;
        return 1;
      }
      next(&info, &idx);
    } while (info <= mInfo[idx]);

    // nothing found to delete
    return 0;
  }

  // reserves space for the specified number of elements. Makes sure the old
  // data fits. exactly the same as reserve(c).
  void rehash(size_t count) {
    // forces a reserve
    reserve(count, true);
  }

  // reserves space for the specified number of elements. Makes sure the old
  // data fits. Exactly the same as rehash(c). Use rehash(0) to shrink to fit.
  void reserve(size_t count) {
    // reserve, but don't force rehash
    reserve(count, false);
  }

  size_type size() const noexcept {  // NOLINT(modernize-use-nodiscard)
    return mNumElements;
  }

  size_type max_size() const noexcept {  // NOLINT(modernize-use-nodiscard)
    return static_cast<size_type>(-1);
  }

  [[nodiscard]] bool empty() const noexcept { return !bool_cast(mNumElements); }

  // float operations for kernel not implemented
  // float max_load_factor() const noexcept {  //
  // NOLINT(modernize-use-nodiscard)
  //  ROBIN_HOOD_TRACE(this)
  //  return MaxLoadFactor100 / 100.0F;
  //}
  //
  //// Average number of elements per bucket. Since we allow only 1 per bucket
  // float load_factor() const noexcept {  // NOLINT(modernize-use-nodiscard)
  //  ROBIN_HOOD_TRACE(this)
  //  return static_cast<float>(size()) / static_cast<float>(mMask + 1);
  //}

  [[nodiscard]] size_t mask() const noexcept { return mMask; }

  [[nodiscard]] size_t calcMaxNumElementsAllowed(
      size_t maxElements) const noexcept {
    if (maxElements <= (numeric_limits<size_t>::max)() / 100) {
      return maxElements * MaxLoadFactor100 / 100;
    }

    // we might be a bit inprecise, but since maxElements is quite large that
    // doesn't matter
    return (maxElements / 100) * MaxLoadFactor100;
  }

  [[nodiscard]] size_t calcNumBytesInfo(size_t numElements) const noexcept {
    // we add a uint64_t, which houses the sentinel (first byte) and padding so
    // we can load 64bit types.
    return numElements + sizeof(uint64_t);
  }

  [[nodiscard]] size_t calcNumElementsWithBuffer(
      size_t numElements) const noexcept {
    auto maxNumElementsAllowed = calcMaxNumElementsAllowed(numElements);
    return numElements +
           (min)(maxNumElementsAllowed, (static_cast<size_t>(0xFF)));
  }

  // calculation only allowed for 2^n values
  [[nodiscard]] size_t calcNumBytesTotal(size_t numElements) const {
#if BITNESS == 64
    return numElements * sizeof(Node) + calcNumBytesInfo(numElements);
#else
    // make sure we're doing 64bit operations, so we are at least safe against
    // 32bit overflows.
    auto const ne = static_cast<uint64_t>(numElements);
    auto const s = static_cast<uint64_t>(sizeof(Node));
    auto const infos = static_cast<uint64_t>(calcNumBytesInfo(numElements));

    auto const total64 = ne * s + infos;
    auto const total = static_cast<size_t>(total64);

    if (static_cast<uint64_t>(total) != total64) {
      throwOverflowError();
    }
    return total;
#endif
  }

 private:
  template <typename Q = mapped_type>
  [[nodiscard]] typename enable_if<!is_void<Q>::value, bool>::type has(
      const value_type& e) const {
    auto it = find(e.first);
    return it != end() && it->second == e.second;
  }

  template <typename Q = mapped_type>
  [[nodiscard]] typename enable_if<is_void<Q>::value, bool>::type has(
      const value_type& e) const {
    return find(e) != end();
  }

  void reserve(size_t c, bool forceRehash) {
    auto const minElementsAllowed = (max)(c, mNumElements);
    auto newSize = InitialNumElements;
    while (calcMaxNumElementsAllowed(newSize) < minElementsAllowed &&
           newSize != 0) {
      newSize *= 2;
    }
    if (!newSize) {
      throwOverflowError();
    }

    // only actually do anything when the new size is bigger than the old one.
    // This prevents to continuously allocate for each reserve() call.
    if (forceRehash || newSize > mMask + 1) {
      rehashPowerOfTwo(newSize);
    }
  }

  // reserves space for at least the specified number of elements.
  // only works if numBuckets if power of two
  void rehashPowerOfTwo(size_t numBuckets) {
    Node* const oldKeyVals = mKeyVals;
    uint8_t const* const oldInfo = mInfo;

    const size_t oldMaxElementsWithBuffer =
        calcNumElementsWithBuffer(mMask + 1);

    // resize operation: move stuff
    init_data(numBuckets);
    if (oldMaxElementsWithBuffer > 1) {
      for (size_t i = 0; i < oldMaxElementsWithBuffer; ++i) {
        if (oldInfo[i] != 0) {
          insert_move(move(oldKeyVals[i]));
          // destroy the node but DON'Ty destroy the data.
          destroy_at(oldKeyVals + i);
        }
      }

      // this check is not necessary as it's guarded by the previous if, but it
      // helps silence g++'s overeager "attempt to free a non-heap object 'map'
      // [-Werror=free-nonheap-object]" warning.
      if (oldKeyVals != reinterpret_cast_no_cast_align_warning<Node*>(&mMask)) {
        // don't destroy old data: put it into the pool instead
        DataPool::addOrFree(oldKeyVals,
                            calcNumBytesTotal(oldMaxElementsWithBuffer));
      }
    }
  }

  [[noreturn]] static void throwOverflowError() {
    throw_exception<overflow_error>(L"robin_hood map overflow");
  }

  template <typename OtherKey, typename... Args>
  pair<iterator, bool> try_emplace_impl(OtherKey&& key, Args&&... args) {
    auto it = find(key);
    if (it == end()) {
      return emplace(piecewise_construct,
                     forward_as_tuple(forward<OtherKey>(key)),
                     forward_as_tuple(forward<Args>(args)...));
    }
    return {it, false};
  }

  template <typename OtherKey, typename Mapped>
  pair<iterator, bool> insert_or_assign_impl(OtherKey&& key, Mapped&& obj) {
    auto it = find(key);
    if (it == end()) {
      return emplace(forward<OtherKey>(key), forward<Mapped>(obj));
    }
    it->second = forward<Mapped>(obj);
    return {it, false};
  }

  void init_data(size_t max_elements) {
    mNumElements = 0;
    mMask = max_elements - 1;
    mMaxNumElementsAllowed = calcMaxNumElementsAllowed(max_elements);

    auto const numElementsWithBuffer = calcNumElementsWithBuffer(max_elements);

    // calloc also zeroes everything
    auto const numBytesTotal = calcNumBytesTotal(numElementsWithBuffer);
    mKeyVals = reinterpret_cast<Node*>(this->allocate_bytes(numBytesTotal));
    memset(mKeyVals, 0, numBytesTotal);
    mInfo = reinterpret_cast<uint8_t*>(mKeyVals + numElementsWithBuffer);

    // set sentinel
    mInfo[numElementsWithBuffer] = 1;

    mInfoInc = InitialInfoInc;
    mInfoHashShift = InitialInfoHashShift;
  }

  template <typename Arg, typename Q = mapped_type>
  typename enable_if<!is_void<Q>::value, Q&>::type doCreateByKey(Arg&& key) {
    while (true) {
      size_t idx{};
      InfoType info{};
      keyToIdx(key, &idx, &info);
      nextWhileLess(&info, &idx);

      // while we potentially have a match. Can't do a do-while here because
      // when mInfo is 0 we don't want to skip forward
      while (info == mInfo[idx]) {
        if (WKeyEqual::operator()(key, mKeyVals[idx].getFirst())) {
          // key already exists, do not insert.
          return mKeyVals[idx].getSecond();
        }
        next(&info, &idx);
      }

      // unlikely that this evaluates to true
      if (mNumElements >= mMaxNumElementsAllowed) {
        increase_size();
        continue;
      }

      // key not found, so we are now exactly where we want to insert it.
      auto const insertion_idx = idx;
      auto const insertion_info = info;
      if (insertion_info + mInfoInc > 0xFF) {
        mMaxNumElementsAllowed = 0;
      }

      // find an empty spot
      while (0 != mInfo[idx]) {
        next(&info, &idx);
      }

      auto& l = mKeyVals[insertion_idx];
      if (idx == insertion_idx) {
        // put at empty spot. This forwards all arguments into the node where
        // the object is constructed exactly where it is needed.
        construct_at(&l, *this, piecewise_construct,
                     forward_as_tuple(forward<Arg>(key)), forward_as_tuple());
      } else {
        shiftUp(idx, insertion_idx);
        l = Node(*this, piecewise_construct,
                 forward_as_tuple(forward<Arg>(key)), forward_as_tuple());
      }

      // mKeyVals[idx].getFirst() = move(key);
      mInfo[insertion_idx] = static_cast<uint8_t>(insertion_info);

      ++mNumElements;
      return mKeyVals[insertion_idx].getSecond();
    }
  }

  // This is exactly the same code as operator[], except for the return values
  template <typename Arg>
  pair<iterator, bool> doInsert(Arg&& keyval) {
    while (true) {
      size_t idx{};
      InfoType info{};
      keyToIdx(getFirstConst(keyval), &idx, &info);
      nextWhileLess(&info, &idx);

      // while we potentially have a match
      while (info == mInfo[idx]) {
        if (WKeyEqual::operator()(getFirstConst(keyval),
                                  mKeyVals[idx].getFirst())) {
          // key already exists, do NOT insert.
          // see http://en.cppreference.com/w/cpp/container/unordered_map/insert
          return make_pair<iterator, bool>(
              iterator(mKeyVals + idx, mInfo + idx), false);
        }
        next(&info, &idx);
      }

      // unlikely that this evaluates to true
      if (mNumElements >= mMaxNumElementsAllowed) {
        increase_size();
        continue;
      }

      // key not found, so we are now exactly where we want to insert it.
      auto const insertion_idx = idx;
      auto const insertion_info = info;
      if (insertion_info + mInfoInc > 0xFF) {
        mMaxNumElementsAllowed = 0;
      }

      // find an empty spot
      while (0 != mInfo[idx]) {
        next(&info, &idx);
      }

      auto& l = mKeyVals[insertion_idx];
      if (idx == insertion_idx) {
        construct_at(&l, *this, forward<Arg>(keyval));
      } else {
        shiftUp(idx, insertion_idx);
        l = Node(*this, forward<Arg>(keyval));
      }

      // put at empty spot
      mInfo[insertion_idx] = static_cast<uint8_t>(insertion_info);

      ++mNumElements;
      return make_pair(
          iterator(mKeyVals + insertion_idx, mInfo + insertion_idx), true);
    }
  }

  bool try_increase_info() {
    if (mInfoInc <= 2) {
      // need to be > 2 so that shift works (otherwise undefined behavior!)
      return false;
    }
    // we got space left, try to make info smaller
    mInfoInc = static_cast<uint8_t>(mInfoInc >> 1U);

    // remove one bit of the hash, leaving more space for the distance info.
    // This is extremely fast because we can operate on 8 bytes at once.
    ++mInfoHashShift;
    auto const numElementsWithBuffer = calcNumElementsWithBuffer(mMask + 1);

    for (size_t i = 0; i < numElementsWithBuffer; i += 8) {
      auto val = unaligned_load<uint64_t>(mInfo + i);
      val = (val >> 1U) & UINT64_C(0x7f7f7f7f7f7f7f7f);
      memcpy(mInfo + i, &val, sizeof(val));
    }
    // update sentinel, which might have been cleared out!
    mInfo[numElementsWithBuffer] = 1;

    mMaxNumElementsAllowed = calcMaxNumElementsAllowed(mMask + 1);
    return true;
  }

  void increase_size() {
    // nothing allocated yet? just allocate InitialNumElements
    if (0 == mMask) {
      init_data(InitialNumElements);
      return;
    }

    auto const maxNumElementsAllowed = calcMaxNumElementsAllowed(mMask + 1);
    if (mNumElements < maxNumElementsAllowed && try_increase_info()) {
      return;
    }

    // it seems we have a really bad hash function! don't try to resize again
    if (mNumElements * 2 < calcMaxNumElementsAllowed(mMask + 1)) {
      throwOverflowError();
    }

    rehashPowerOfTwo((mMask + 1) * 2);
  }

  void destroy() {
    if (!mMask) {
      // don't deallocate!
      return;
    }

    Destroyer<Self, IsFlat && is_trivially_destructible<Node>::value>{}
        .nodesDoNotDeallocate(*this);

    // This protection against not deleting mMask shouldn't be needed as it's
    // sufficiently protected with the 0==mMask check, but I have this anyways
    // because g++ 7 otherwise reports a compile error: attempt to free a
    // non-heap object 'fm'
    // [-Werror=free-nonheap-object]
    if (mKeyVals != reinterpret_cast_no_cast_align_warning<Node*>(&mMask)) {
      this->deallocate_bytes(mKeyVals, mNumElements * sizeof(Node));
    }
  }

  void init() noexcept {
    mKeyVals = reinterpret_cast_no_cast_align_warning<Node*>(&mMask);
    mInfo = reinterpret_cast<uint8_t*>(&mMask);
    mNumElements = 0;
    mMask = 0;
    mMaxNumElementsAllowed = 0;
    mInfoInc = InitialInfoInc;
    mInfoHashShift = InitialInfoHashShift;
  }

  // members are sorted so no padding occurs
  Node* mKeyVals =
      reinterpret_cast_no_cast_align_warning<Node*>(&mMask);  // 8 byte  8
  uint8_t* mInfo = reinterpret_cast<uint8_t*>(&mMask);        // 8 byte 16
  size_t mNumElements = 0;                                    // 8 byte 24
  size_t mMask = 0;                                           // 8 byte 32
  size_t mMaxNumElementsAllowed = 0;                          // 8 byte 40
  InfoType mInfoInc = InitialInfoInc;                         // 4 byte 44
  InfoType mInfoHashShift =
      InitialInfoHashShift;  // 4 byte 48
                             // 16 byte 56 if NodeAllocator
};

}  // namespace un::details
}  // namespace ktl