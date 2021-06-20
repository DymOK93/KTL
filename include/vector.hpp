#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <vector>
namespace ktl {
using std::vector;
}
#else
#include <basic_types.h>
#include <exception.h>
#include <algorithm.hpp>
#include <allocator.hpp>
#include <assert.hpp>
#include <compressed_pair.hpp>
#include <iterator.hpp>
#include <memory_tools.hpp>
#include <smart_pointer.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

namespace ktl {
template <class Ty, class Allocator = basic_paged_allocator<Ty> >
class vector {
 public:
  using value_type = Ty;

  using allocator_type = Allocator;
  using allocator_traits_type = allocator_traits<allocator_type>;
  using pointer = typename allocator_traits_type::pointer;
  using const_pointer = typename allocator_traits_type::const_pointer;
  using reference = typename allocator_traits_type::reference;
  using const_reference = typename allocator_traits_type::const_reference;
  using size_type = typename allocator_traits_type::size_type;

  using iterator = pointer;
  using const_iterator = const_pointer;

 private:
  struct Impl {
    constexpr explicit Impl() noexcept = default;

    constexpr Impl(const Impl&) noexcept = default;

    constexpr Impl(Impl&& other) noexcept
        : buffer{other.buffer}, size{other.size}, capacity{other.capacity} {
      other.buffer = nullptr;
      other.size = 0;
      other.capacity = 0;
    }

    constexpr Impl& operator=(const Impl&) noexcept = default;

    constexpr Impl& operator=(Impl&& other) noexcept {
      if (addressof(other) != this) {
        buffer = other.buffer;
        other.buffer = nullptr;
        size = other.size;
        other.size = 0;
        capacity = other.capacity;
        other.capacity = 0;
      }
      return *this;
    }

    pointer buffer{nullptr};
    size_type size{0};
    size_type capacity{0};
  };

 public:
  static_assert(is_same_v<Ty, typename allocator_traits_type::value_type>,
                "Incompatible allocator");

 private:
  template <class InputIt>
  using forward_or_greater_t =
      is_base_of<typename iterator_traits<InputIt>::iterator_category,
                 forward_iterator_tag>;

 private:
  static constexpr size_type MIN_CAPACITY{1}, GROWTH_MULTIPLIER{2};

 public:
  vector() noexcept(is_nothrow_default_constructible_v<allocator_type>) =
      default;

  explicit vector(const allocator_type& alloc) noexcept(
      is_nothrow_constructible_v<allocator_type, const allocator_type&>)
      : m_impl{one_then_variadic_args{}, alloc} {}

  explicit vector(allocator_type&& alloc) noexcept(
      is_nothrow_constructible_v<allocator_type, allocator_type&&>)
      : m_impl{one_then_variadic_args{}, move(alloc)} {}

  template <class Alloc = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Alloc>, int> = 1>
  vector(Alloc&& alloc = Alloc{}) noexcept(
      is_nothrow_constructible_v<allocator_type, Alloc>)
      : m_impl{one_then_variadic_args{}, forward<Alloc>(alloc)} {}

  template <class Alloc = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Alloc> &&
                            is_default_constructible_v<value_type>,
                        int> = 0>
  explicit vector(size_type object_count, Alloc&& alloc = Alloc{}) noexcept(
      is_nothrow_constructible_v<allocator_type, Alloc>&&
          is_nothrow_default_constructible_v<value_type>)
      : m_impl{one_then_variadic_args{}, forward<Alloc>(alloc)} {
    grow<true>(object_count, make_default_construct_helper(0),
               make_dummy_transfer(), object_count);
  }

  template <class Alloc = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Alloc> &&
                            is_copy_constructible_v<value_type>,
                        int> = 0>
  explicit vector(
      size_type object_count,
      const Ty& value,
      Alloc&& alloc =
          Alloc{}) noexcept(is_nothrow_constructible_v<allocator_type, Alloc>&&
                                is_nothrow_default_constructible_v<Ty>)
      : m_impl{one_then_variadic_args{}, forward<Alloc>(alloc)} {
    grow<true>(object_count, make_construct_fill_helper(0),
               make_dummy_transfer(), value, object_count);
  }

  template <class InputIt,
            class Alloc = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Alloc> &&
                            is_constructible_v<
                                value_type,
                                typename iterator_traits<InputIt>::reference>,
                        int> = 0>
  vector(InputIt first, InputIt last, Alloc&& alloc = Alloc{})
      : m_impl{one_then_variadic_args{}, forward<Alloc>(alloc)} {
    assign_range_impl<true>(first, last, forward_or_greater_t<InputIt>{});
  }

  vector(const vector& other)
      : m_impl{one_then_variadic_args{},
               allocator_traits_type::select_on_container_copy_construction(
                   other.get_alloc())} {
    grow_unchecked<true>(other.size(), make_cloner(), make_dummy_transfer(),
                         other);
  }

  template <class Alloc = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Alloc>, int> = 0>
  vector(const vector& other, Alloc&& alloc)
      : m_impl{one_then_variadic_args{}, forward<Alloc>(alloc)} {
    grow_unchecked<true>(other.size(), make_cloner(), make_dummy_transfer(),
                         other);
  }

  vector(vector&& other) noexcept(
      is_nothrow_move_constructible_v<allocator_type>)
      : m_impl{move(other.m_impl)} {}

  template <class Alloc = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Alloc>, int> = 0>
  vector(vector&& other, Alloc&& alloc)
      : m_impl{one_then_variadic_args{}, forward<Alloc>(alloc)} {
    if constexpr (is_same_v<allocator_type, remove_cv_t<Alloc> > &&
                  allocator_traits_type::is_always_equal::value) {
      m_impl.get_second() = move(other.m_impl.get_second());
    } else {
      if (other.get_alloc() == alloc) {
        m_impl.get_second() = move(other.m_impl.get_second());
      } else {
        assign(other);
        other.clear();
      }
    }
  }

  vector& operator=(const vector& other) {
    using propagate_on_copy_assignment_t =
        typename allocator_traits_type::propagate_on_container_copy_assignment;
    if (addressof(other) != this) {
      copy_assignment_impl(other, propagate_on_copy_assignment_t{});
    }
    return *this;
  }

  vector& operator=(vector&& other) {
    using propagate_on_move_assignment_t =
        typename allocator_traits_type::propagate_on_container_move_assignment;
    if (addressof(other) != this) {
      move_assignment_impl(move(other), propagate_on_move_assignment_t{});
    }
    return *this;
  }

  ~vector() noexcept { destroy_and_deallocate(); }

  void assign(size_type count, const Ty& value) {
    if (count > capacity()) {
      grow<true>(count, make_construct_fill_helper(0), make_dummy_transfer(),
                 value, count);
    } else {
      auto last{fill(data(), data() + count, value)};
      destroy(last, data() + size());
      get_size() = count;
    }
  }

  template <class InputIt>
  void assign(InputIt first, InputIt last) {
    assign_range_impl<true>(first, last, forward_or_greater_t<InputIt>{});
  }

  const allocator_type& get_allocator() const noexcept { return get_alloc(); }

  reference at(size_type idx) { return at_index_verified(data(), size(), idx); }

  const_reference at(size_type idx) const {
    return at_index_verified(data(), size(), idx);
  }

  reference operator[](size_type idx) noexcept {
    assert_with_msg(idx < size(), L"index is out of range");
    return data()[idx];
  }

  const_reference operator[](size_type idx) const noexcept {
    assert_with_msg(idx < size(), L"index is out of range");
    return data()[idx];
  }

  reference front() noexcept {
    assert_with_msg(!empty(), L"front() called at empty vector");
    return data()[0];
  }

  reference back() noexcept {
    assert_with_msg(!empty(), L"back() called at empty vector");
    return data()[size() - 1];
  }

  const_reference front() const noexcept {
    assert_with_msg(!empty(), L"front() called at empty vector");
    return data()[0];
  }

  const_reference back() const noexcept {
    assert_with_msg(!empty(), L"back() called at empty vector");
    return data()[size() - 1];
  }

  pointer data() noexcept { return get_buffer(); }
  const_pointer data() const noexcept { return get_buffer(); }

  iterator begin() noexcept { return data(); }
  iterator end() noexcept { return data() + size(); }

  const_iterator begin() const noexcept { return cbegin(); }
  const_iterator end() const noexcept { return cend(); }
  const_iterator cbegin() const noexcept { return data(); }
  const_iterator cend() const noexcept { return data() + size(); }

  bool empty() const noexcept { return size() == 0; }
  size_type size() const noexcept { return get_size(); }

  constexpr size_type max_size() const noexcept {
    return (numeric_limits<size_type>::max)() - 1;
  }

  void reserve(size_type new_capacity) {
    if (new_capacity > capacity()) {
      grow<false>(new_capacity, make_dummy_construct_helper(),
                  make_transfer_without_shift(), size());
    }
  }

  size_type capacity() const noexcept { return get_capacity(); }

  void shrink_to_fit() {
    const size_type current_size{size()};
    if (capacity() > current_size) {
      grow<false>(current_size, make_dummy_construct_helper(),
                  make_transfer_without_shift(), current_size);
    }
  }

  void clear() noexcept {
    destroy_n(begin(), size());
    get_size() = 0;
  }

  iterator insert(const_iterator pos, const Ty& value) {
    return emplace(pos, value);
  }

  iterator insert(const_iterator pos, Ty&& value) {
    return emplace(pos, move(value));
  }

  iterator insert(const_iterator pos, size_type count, const Ty& value) {
    if (pos == end()) {
      return append_n(count, value);
    }

    const size_type current_size{size()};
    const auto offset{static_cast<size_t>(pos - begin())};

    if (const size_type required = current_size + count;
        required > capacity()) {
      grow<true>(
          calc_optimal_growth(required), make_construct_fill_helper(offset),
          make_transfer_with_shift_right(offset, offset + count), value, count);
    } else {
      Ty tmp{value};  // It's required to construct value to avoid moved-from
                      // state aster shift
      auto* first{data() + offset};
      make_transfer_without_shift()(first + count, current_size - offset,
                                    first);
      get_size() += count;
      fill(first, first + count, tmp);
    }
    return begin() + offset;
  }

  // TODO: SFINAE
  // template <class InputIt>
  // iterator insert(const_iterator pos, InputIt first, InputIt last);

  template <class... Types>
  iterator emplace(const_iterator pos, Types&&... args) {
    if (pos == cend()) {
      emplace_back(forward<Types>(args)...);
      return prev(end());
    }

    const size_type current_size{size()};
    const auto offset{static_cast<size_type>(pos - begin())};

    if (current_size == capacity()) {
      grow<true>(calc_growth(), make_construct_at_helper(offset),
                 make_transfer_with_shift_right(offset, offset + 1),
                 forward<Types>(args)...);
    } else {
      Ty tmp{forward<Types>(args)...};
      value_type* buffer{data()};
      make_construct_at_helper(current_size)(buffer, move_if_noexcept(back()));
      ++get_size();
      shift_right(begin() + offset, prev(end()), 1);
      make_construct_at_helper(offset)(buffer, move(tmp));
    }
    return begin() + offset;
  }

  iterator erase(const_iterator pos) {
    auto range_end{end()};
    if (pos == range_end) {
      return range_end;
    }
    return erase_impl(pos, 1);
  }

  iterator erase(const_iterator first, const_iterator last) {
    const auto count{static_cast<size_type>(last - first)};
    if (first == last) {
      return begin() + count;
    }
    return erase_impl(first, count);
  }

  void push_back(const Ty& value) { emplace_back(value); }

  void push_back(Ty&& value) { emplace_back(move(value)); }

  template <class... Types>
  reference emplace_back(Types&&... args) {
    const size_type current_size{size()};
    if (current_size == capacity()) {
      grow<true>(calc_growth(), make_construct_at_helper(current_size),
                 make_transfer_without_shift(), forward<Types>(args)...);
    } else {
      make_construct_at_helper(current_size)(data(), forward<Types>(args)...);
      ++get_size();
    }
    return back();
  }

  void pop_back() {
    assert_with_msg(!empty(), L"pop_back() called at empty vector");
    destroy_at(addressof(back()));
    --get_size();
  }

  void resize(size_type count) {
    resize_impl(
        count, make_default_construct_helper(size()),
        count - size());  // Unsigned underflow is allowed by C++ Standard. If
                          // count < size(), uninitialized_default_construct_n()
                          // won't be called
  }

  void resize(size_type count, const Ty& value) {
    resize_impl(count, make_construct_fill_helper(size()), value,
                count - size());
  }

  void swap(vector& other) noexcept(
      allocator_traits_type::propagate_on_container_swap::value ||
      allocator_traits_type::is_always_equal::value) {
    if (addressof(other) != this) {
      swap_impl(other,
                typename allocator_traits_type::propagate_on_container_swap{});
    }
  }

 private:
  void destroy_and_deallocate() {
    destroy_n(begin(), size());
    deallocate_buffer(get_alloc(), data(), capacity());
  }

  template <bool VerifyRangeLength, class InputIt>
  void assign_range_impl(InputIt first, InputIt last, false_type) {
    for (; first != last; first = next(first)) {
      push_back(*first);
    }
  }

  template <bool VerifyRangeLength, class ForwardIt>
  void assign_range_impl(ForwardIt first, ForwardIt last, true_type) {
    const size_type current_size{size()},
        range_length{static_cast<size_type>(distance(first, last))};
    if (range_length > capacity()) {
      clear();
      grow_with_optional_check_and_adjust_resize<VerifyRangeLength>(
          range_length, make_range_construct_helper(0), make_dummy_transfer(),
          first, range_length);
    } else if (range_length <= current_size) {
      auto my_first{begin()};
      copy(first, last, my_first);
      destroy(my_first + range_length, end());
      get_size() = range_length;
    } else {
      auto subrange_end{first};
      advance(first, current_size);
      copy(first, subrange_end, begin());
      make_copy_helper()(data() + current_size, range_length - current_size,
                         subrange_end);
      get_size() = range_length;
    }
  }

  void copy_assignment_impl(const vector& other, true_type) {
    if constexpr (allocator_traits_type::is_always_equal::value) {
      copy_assignment_impl_equal_allocators(other);
    } else {
      if (get_alloc() == other.get_alloc()) {
        copy_assignment_impl_equal_allocators(other);
      } else {
        destroy_and_deallocate();
        m_impl.get_second() = Impl{};  // Explicitly fill by zeros pointer to
                                       // old buffer, size and capacity fields
        get_alloc() = other.get_alloc();
        grow_unchecked<true>(other.size(), make_cloner(), make_dummy_transfer(),
                             other);
      }
    }
  }

  void copy_assignment_impl_equal_allocators(const vector& other) {
    get_alloc() = other.get_alloc();
    copy_assignment_impl(other, false_type{});
  }

  void copy_assignment_impl(const vector& other, false_type) {
    assign_range_impl<false>(other.begin(), other.end(), true_type{});
  }

  void move_assignment_impl(vector&& other, true_type) {
    destroy_and_deallocate();
    get_alloc() = move(other.get_alloc());
    m_impl.get_second() = move(other.m_impl.get_second());
  }

  void move_assignment_impl_equal_allocators(vector&& other) {
    destroy_and_deallocate();
    m_impl.get_second() = move(other.m_impl.get_second());
  }

  void move_assignment_impl(vector&& other, false_type) {
    if constexpr (allocator_traits_type::is_always_equal::value) {
      move_assignment_impl_equal_allocators(other);
    } else {
      if (get_alloc() == other.get_alloc()) {
        move_assignment_impl_equal_allocators(other);
      } else {  // With move_iterator it's possible to lose optimization for the
                // trivially copyable types
        const size_type other_size{other.size()};
        if (other_size > capacity()) {
          grow_unchecked(other_size, make_taker(),
                         make_transfer_without_shift(), move(other));
        } else {
          auto last{move(other.begin(), other.end(), begin())};
          destroy(last, end());
          get_size() = other_size;
        }
        other.clear();
      }
    }
  }

  iterator append_n(size_type count, const Ty& value) {
    const size_type current_size{size()};
    if (const size_type required = current_size + count;
        required > capacity()) {
      grow<true>(calc_optimal_growth(required),
                 make_construct_fill_helper(current_size),
                 make_transfer_without_shift(), value, count);
    } else {
      make_construct_fill_helper(current_size)(data() + current_size, value,
                                               count);
      get_size() += count;
    }
    return begin() + current_size;
  }

  iterator erase_impl(const_iterator pos, size_type count) {
    auto range_begin{begin()};
    assert_with_msg(pos >= range_begin && pos + count <= end(),
                    L"iterator does not belong to the vector");
    const auto offset{pos - range_begin};
    shift_left(range_begin + offset, end(), count);
    get_size() -= count;
    return range_begin + offset;
  }

  template <class ConstructionPolicy, class... Types>
  void resize_impl(size_type count,
                   ConstructionPolicy construction_handler,
                   Types&&... args) {
    const size_type current_size{size()};
    if (count < current_size) {
      destroy_n(begin() + count, current_size - count);
    } else if (count > current_size) {
      if (count <= capacity()) {
        construction_handler(data(), forward<Types>(args)...);
      } else {
        grow<false>(count, construction_handler, make_transfer_without_shift(),
                    forward<Types>(args)...);
      }
    }
    get_size() = count;
  }

  void swap_impl(vector& other, true_type) noexcept {
    ::swap(m_impl, other.m_impl);
  }

  void swap_impl(vector& other, false_type) noexcept(
      allocator_traits_type::is_always_equal::value) {
    if constexpr (allocator_traits_type::is_always_equal::value) {
      ::swap(m_impl.get_second().other.m_impl.get_second());
    } else {
      assert_with_msg(
          get_alloc() == other.get_alloc(),
          "vectors are not swappable due to incompatible allocators");
      ::swap(m_impl.get_second(), other.m_impl.get_second());
    }
  }

  allocator_type& get_alloc() noexcept { return m_impl.get_first(); }

  const allocator_type& get_alloc() const noexcept {
    return m_impl.get_first();
  }

  const_pointer get_buffer() const noexcept {
    return m_impl.get_second().buffer;
  }

  pointer& get_buffer() noexcept { return m_impl.get_second().buffer; }

  size_type get_size() const noexcept { return m_impl.get_second().size; }

  size_type& get_size() noexcept { return m_impl.get_second().size; }

  size_type get_capacity() const noexcept {
    return m_impl.get_second().capacity;
  }

  size_type& get_capacity() noexcept { return m_impl.get_second().capacity; }

  template <bool VerifyNewCapacity,
            class TransferPolicy,
            class ConstructPolicy,
            class... Types>
  void grow_with_optional_check_and_adjust_resize(
      size_type new_capacity,
      ConstructPolicy construction_handler,
      TransferPolicy transfer_handler,
      Types&&... args) {
    if constexpr (VerifyNewCapacity) {
      grow<true>(new_capacity, construction_handler, transfer_handler,
                 forward<Types>(args)...);
    } else {
      grow_unchecked<true>(new_capacity, construction_handler, transfer_handler,
                           forward<Types>(args)...);
    }
  }

  template <bool AdjustSize,
            class TransferPolicy,
            class ConstructPolicy,
            class... Types>
  void grow(size_type new_capacity,
            ConstructPolicy construction_handler,
            TransferPolicy transfer_handler,
            Types&&... args) {
    throw_exception_if_not<length_error>(new_capacity <= max_size(),
                                         L"vector is too large");
    grow_unchecked<AdjustSize>(new_capacity, construction_handler,
                               transfer_handler, forward<Types>(args)...);
  }

  template <bool AdjustSize,
            class TransferPolicy,
            class ConstructPolicy,
            class... Types>
  void grow_unchecked(size_type new_capacity,
                      ConstructPolicy construction_handler,
                      TransferPolicy transfer_handler,
                      Types&&... args) {
    auto& alc{get_alloc()};
    value_type* old_buffer{data()};
    const size_type old_size{size()};
    value_type* new_buffer{allocate_buffer(alc, new_capacity)};

    auto alc_guard{make_alloc_temporary_guard(new_buffer, alc, new_capacity)};
    const size_type size_adjustment{
        construction_handler(new_buffer, forward<Types>(args)...)};
    transfer_handler(new_buffer, old_size, old_buffer);
    destroy_and_deallocate();
    alc_guard.release();

    if constexpr (AdjustSize) {
      get_size() += size_adjustment;
    }

    get_buffer() = new_buffer;
    get_capacity() = new_capacity;
  }

  constexpr size_type calc_optimal_growth(size_type required) noexcept {
    return (max)(calc_growth(), required);
  }

  constexpr size_type calc_growth() noexcept {
    const size_type current_capacity{capacity()}, max_capacity{max_size()};
    if (current_capacity == max_capacity) {
      return max_capacity + 1;
    }
    return (min)(max_capacity, calc_growth_unchecked(current_capacity));
  }

  static constexpr size_type calc_growth_unchecked(
      size_type old_capacity) noexcept {
    return old_capacity == 0 ? MIN_CAPACITY : old_capacity * GROWTH_MULTIPLIER;
  }

  static constexpr auto make_dummy_construct_helper() {
    return []([[maybe_unused]] value_type* buffer, size_t old_size) {
      return old_size;
    };
  }

  static constexpr auto make_construct_at_helper(size_type pos) {
    return [pos](value_type* buffer, auto&&... args) {
      construct_at(buffer + pos, forward<decltype(args)>(args)...);
      return 1u;
    };
  }

  static constexpr auto make_range_construct_helper(size_type pos) {
    return [pos](value_type* buffer, auto first, size_type count) {
      make_copy_helper()(buffer + pos, count, first);
      return count;
    };
  }

  static constexpr auto make_default_construct_helper(size_type pos) {
    return [pos](value_type* buffer, size_type count) {
      uninitialized_default_construct_n(buffer + pos, count);
      return count;
    };
  }

  static constexpr auto make_construct_fill_helper(size_type pos) {
    return [pos](value_type* buffer, const Ty& value, size_t count) {
      uninitialized_fill_n(buffer + pos, count, value);
      return count;
    };
  }

  static constexpr auto make_copy_helper() noexcept {
    return [](value_type* dst, size_type count, auto src_it) noexcept {
      uninitialized_copy_n(src_it, count, dst);
    };
  }

  static constexpr auto make_move_helper() noexcept {
    return [](value_type* dst, size_type count, auto src_it) noexcept {
      uninitialized_move_n(src_it, count, dst);
    };
  }

  static constexpr auto make_cloner() noexcept {
    return [](value_type* dst, const vector& other) noexcept {
      const size_type object_count{other.size()};
      make_copy_helper()(dst, object_count, other.data());
      return object_count;
    };
  }

  static constexpr auto make_taker() noexcept {
    return [](value_type* dst, vector&& other) noexcept {
      const size_type object_count{other.size()};
      make_move_helper()(dst, object_count, other.data());
      return object_count;
    };
  }

  static constexpr auto make_transfer_without_shift() noexcept {
    if constexpr (is_nothrow_move_constructible_v<value_type> ||
                  !is_copy_constructible_v<value_type>) {
      return [](value_type* dst, size_type count, const value_type* src) {
        make_move_helper()(dst, count, src);
      };
    } else {
      return [](value_type* dst, size_type count, const value_type* src) {
        make_copy_helper()(dst, count, src);
      };
    }
  }

  static constexpr auto make_transfer_with_shift_right(
      size_type left_bound,
      size_type right_bound) noexcept {
    return [left_bound, right_bound](value_type* dst, size_type count,
                                     const value_type* src) {
      make_transfer_without_shift()(dst, left_bound, src);
      make_transfer_without_shift()(dst + right_bound, count - left_bound,
                                    src + left_bound);
    };
  }

  static constexpr auto make_dummy_transfer() noexcept {
    return
        []([[maybe_unused]] value_type* dst, [[maybe_unused]] size_type count,
           [[maybe_unused]] const value_type* src) noexcept {};
  }

  static value_type* allocate_buffer(allocator_type& alc, size_type obj_count) {
    return allocator_traits_type::allocate(alc, obj_count);
  }

  static void deallocate_buffer(allocator_type& alc,
                                value_type* buffer,
                                size_type obj_count) noexcept {
    if constexpr (allocator_traits_type::enable_delete_null::value) {
      allocator_traits_type::deallocate(alc, buffer, obj_count);
    } else {
      if (buffer) {
        allocator_traits_type::deallocate(alc, buffer, obj_count);
      }
    }
  }

  template <class ValueTy>
  static decltype(auto) at_index_verified(ValueTy* data,
                                          size_type size,
                                          size_type idx) {
    throw_exception_if_not<out_of_range>(idx < size, L"index is out of range",
                                         constexpr_message_tag{});
    return data[idx];
  }

 private:
  compressed_pair<allocator_type, Impl> m_impl;
};

template <class Ty, class Allocator>
void swap(vector<Ty, Allocator>& lhs,
          vector<Ty, Allocator>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
  lhs.swap(rhs);
}

template <class Ty, class Allocator>
bool operator==(const vector<Ty, Allocator>& lhs,
                const vector<Ty, Allocator>& rhs) {
  return lhs.size() == rhs.size() && equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <class Ty, class Allocator>
bool operator!=(const vector<Ty, Allocator>& lhs,
                const vector<Ty, Allocator>& rhs) {
  return !(lhs == rhs);
}

template <class Ty, class Allocator>
bool operator<(const vector<Ty, Allocator>& lhs,
               const vector<Ty, Allocator>& rhs) {
  return lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                                 less<Ty>{});
}

template <class Ty, class Allocator>
bool operator<=(const vector<Ty, Allocator>& lhs,
                const vector<Ty, Allocator>& rhs) {
  return !(lhs > rhs);
}

template <class Ty, class Allocator>
bool operator>(const vector<Ty, Allocator>& lhs,
               const vector<Ty, Allocator>& rhs) {
  return rhs < lhs;
}

template <class Ty, class Allocator>
bool operator>=(const vector<Ty, Allocator>& lhs,
                const vector<Ty, Allocator>& rhs) {
  return !(lhs < rhs);
}
}  // namespace ktl
#endif