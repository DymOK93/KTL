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
    constexpr Impl() noexcept = default;

    constexpr Impl(size_t count) noexcept : size{count}, capacity{count} {}

    constexpr Impl(pointer ptr, size_t count) noexcept
        : buffer{ptr}, size{count}, capacity{count} {}

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
  using forward_or_greater_t = bool_constant<
      is_same_v<typename iterator_traits<InputIt>::iterator_category,
                forward_iterator_tag> ||
      is_same_v<typename iterator_traits<InputIt>::iterator_category,
                random_access_iterator_tag> >;

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
    grow(object_count, make_default_construct_helper(object_count),
         make_dummy_transfer());
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
    grow(object_count, make_fill_helper(object_count), make_dummy_transfer(),
         value);
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
    append_range_impl<false_type>(first, last, forward_or_greater_t<InputIt>{});
  }

  // vector(const vector& other);

  vector(vector&& other) noexcept(
      is_nothrow_move_constructible_v<allocator_type>)
      : m_impl{move(other.m_impl)} {}

  template <class InputIt, class Alloc = allocator_type>
  vector(vector&& other, Alloc&& alloc);

  ~vector() noexcept { destroy_and_deallocate(); }

  bool empty() const noexcept { return size() == 0; }
  size_type size() const noexcept { return get_size(); }

  constexpr size_type max_size() const noexcept {
    return (numeric_limits<size_type>::max)() - 1;
  }

  size_type capacity() const noexcept { return get_capacity(); }

  pointer data() noexcept { return get_buffer(); }
  const_pointer data() const noexcept { get_buffer(); }

  reference operator[](size_type idx) noexcept {
    assert_with_msg(idx < size(), L"index is out of range");
    return data()[idx];
  }

  const_reference operator[](size_type idx) const noexcept {
    assert_with_msg(idx < size(), L"index is out of range");
    return data()[idx];
  }

  reference at(size_type idx) { return at_index_verified(data(), size(), idx); }

  const_reference at(size_type idx) const {
    return at_index_verified(data(), size(), idx);
  }

  iterator begin() noexcept { return data(); }
  iterator end() noexcept { return data() + size(); }

  const_iterator begin() const noexcept { return cbegin(); }
  const_iterator end() const noexcept { return cend(); }
  const_iterator cbegin() const noexcept { return data(); }
  const_iterator cend() const noexcept { return data() + size(); }

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

  void push_back(const Ty& value) { emplace_back(value); }

  void push_back(Ty&& value) { emplace_back(move(value)); }

  template <class... Types>
  reference emplace_back(Types&&... args) {
    const size_type current_size{size()};
    if (current_size == capacity()) {
      grow(calc_growth(), make_emplace_helper(current_size),
           make_transfer_without_shift(), forward<Types>(args)...);
    } else {
      make_emplace_helper(current_size)(data(), forward<Types>(args)...);
      ++get_size();
    }
    return back();
  }

  void pop_back() {
    assert_with_msg(!empty(), L"pop_back() called at empty vector");
    destroy_at(addressof(back()));
    --get_size();
  }

  void clear() noexcept {
    destroy_n(begin(), size());
    get_size() = 0;
  }

 private:
  void destroy_and_deallocate() {
    destroy_n(begin(), size());
    allocator_traits_type::deallocate(get_alloc(), data(), capacity());
  }

  template <typename GrowthTypeTag, class InputIt>
  void append_range_impl(InputIt first, InputIt last, false_type) {
    for (; first != last; first = next(first)) {
      push_back(*first);
    }
  }

  template <typename GrowthTypeTag, class ForwardIt>
  void append_range_impl(ForwardIt first, ForwardIt last, true_type) {
    const auto range_length{static_cast<size_type>(distance(first, last))};
    append_range_by_forward_iterators<GrowthTypeTag>(first, range_length);
  }

  template <typename GrowthTypeTag, class ForwardIt>
  void append_range_by_forward_iterators(ForwardIt first,
                                         size_type range_length) {
    const size_type current_size{size()},
        required_capacity{current_size + range_length};
    if (required_capacity < capacity()) {
      make_copy_helper()(data() + current_size, range_length, first);
      get_size() += range_length;
    } else {
      const size_type new_capacity{
          select_strategy_and_calc_growth(required_capacity, GrowthTypeTag{})};
      grow(new_capacity, make_range_construct_helper(current_size),
           make_transfer_without_shift(), first, range_length);
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

  template <class TransferPolicy, class ConstructPolicy, class... Types>
  void grow(size_type new_capacity,
            ConstructPolicy construction_handler,
            TransferPolicy transfer_handler,
            Types&&... args) {
    throw_exception_if_not<length_error>(new_capacity <= max_size(),
                                         L"vector is too large");
    grow_unchecked(new_capacity, construction_handler, transfer_handler,
                   forward<Types>(args)...);
  }

  template <class TransferPolicy, class ConstructPolicy, class... Types>
  void grow_unchecked(size_type new_capacity,
                      ConstructPolicy construction_handler,
                      TransferPolicy transfer_handler,
                      Types&&... args) {
    auto& alc{get_alloc()};
    value_type* old_buffer{data()};
    const size_type old_size{size()};
    value_type* new_buffer{allocate_buffer(alc, new_capacity)};

    auto alc_guard{make_alloc_temporary_guard(new_buffer, alc, new_capacity)};
    size_t size_adjustment{
        construction_handler(new_buffer, forward<Types>(args)...)};
    transfer_handler(new_buffer, old_size, old_buffer);
    destroy_and_deallocate();
    alc_guard.release();

    get_buffer() = new_buffer;
    get_size() += size_adjustment;
    get_capacity() = new_capacity;
  }

  constexpr size_type select_strategy_and_calc_growth(size_type required,
                                                      false_type) noexcept {
    return required;
  }

  constexpr size_type select_strategy_and_calc_growth(size_type required,
                                                      true_type) noexcept {
    return calc_optimal_growth(required);
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

  static constexpr auto make_emplace_helper(size_type pos) {
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

  static constexpr auto make_default_construct_helper(size_type count) {
    return [count](value_type* buffer) {
      uninitialized_default_construct_n(buffer, count);
      return count;
    };
  }

  static constexpr auto make_fill_helper(size_type count) {
    return [count](value_type* buffer, const Ty& value) {
      uninitialized_fill_n(buffer, count, value);
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
    return allocator_traits_type::deallocate(alc, buffer, obj_count);
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
};  // namespace ktl

// template <class Ty>
// vector<Ty>::vector(const vector& other)
//    : m_data(other.m_size), m_size{other.m_size} {
//  std::uninitialized_copy_n(other.m_data.begin(), other.m_size,
//  m_data.begin());
//}
//
// template <class Ty>
// vector<Ty>& vector<Ty>::operator=(const vector& other) {
//  if (this != std::addressof(other)) {
//    if (other.m_size >
//        Capacity()) {  //Если размер копируемого вектора больше, чем емкость
//                       //текущего - не реаллоцируем уже имеющиеся элементы
//      vector tmp(other);  // Copy-and-Swap
//      Swap(tmp);
//    } else {
//      const size_type min_size{std::min(m_size, other.m_size)};
//      std::copy(  //Для примитивных типов std::copy использует memmove()
//          other.m_data.begin(), other.m_data.begin() + min_size,
//          m_data.begin());
//      if (min_size == other.m_size) {  //Текущий вектор больше копируемого
//        std::destroy(m_data.begin() + min_size, m_data.begin() + m_size);
//      } else {
//        std::uninitialized_copy_n(other.m_data.begin() + min_size,
//                                  other.m_size - min_size,
//                                  m_data.begin() + min_size);
//      }
//    }
//    m_size = other.m_size;
//  }
//  return *this;
//}

//
// template <class Ty>
// void vector<Ty>::Reserve(size_type n) {
//  if (n > Capacity()) {
//    raw_buffer tmp(n);
//    move_if_noexcept_or_copy_buf(tmp, m_data, m_size);
//    destroy_and_swap(m_data, m_size, tmp);
//  }
//}
//
// template <class Ty>
// void vector<Ty>::Resize(size_type n) {
//  Reserve(n);
//  if (n < m_size) {
//    std::destroy(m_data.begin() + n, m_data.begin() + m_size);
//  } else {
//    std::uninitialized_default_construct(  //Конструирование объектов по
//                                           //умолчанию
//        m_data.begin() + m_size, m_data.begin() + n);
//  }
//  m_size = n;
//}
//
// template <class Ty>
// template <typename... Types>
// Ty& vector<Ty>::EmplaceBack(Types&&... args) {
//  Ty& elem_ref{emplace_back_helper(std::forward<Types>(args)...)};
//  ++m_size;
//  return elem_ref;
//}
//
// template <class Ty>
// typename vector<Ty>::iterator vector<Ty>::Insert(const_iterator pos,
//                                                 const Ty& elem) {
//  return Emplace(pos, elem);
//}
//
// template <class Ty>
// typename vector<Ty>::iterator vector<Ty>::Insert(const_iterator pos,
//                                                 Ty&& elem) {
//  return Emplace(pos, std::move(elem));
//}
//
// template <class Ty>
// template <typename... Types>
// typename vector<Ty>::iterator vector<Ty>::Emplace(const_iterator it,
//                                                  Types&&... args) {
//  auto offset{it - m_data.begin()};
//  if (offset == m_size) {  //Смещение от начала
//    emplace_back_helper(std::forward<Types>(args)...);
//  } else {
//    if (m_size < Capacity()) {
//      emplace_back_helper(std::move_if_noexcept(
//          m_data[m_size - 1]));  //Конструируем собственный последний
//          элемент на
//                                 // 1 позицию правее
//      shift_right(
//          m_data.begin() + offset,
//          m_data.begin() + m_size  //Значение m_size пока остается прежним
//      );
//      construct_in_place(std::addressof(m_data[offset]),
//                         std::forward<Types>(args)...);
//    } else {
//      raw_buffer tmp(calculate_new_capacity(  //Вычисляем требуемую емкость
//          Capacity(), m_size + 1));
//      construct_in_place(std::addressof(tmp[offset]),
//                         std::forward<Types>(args)...);  //Конструируем
//                         элемент
//      move_if_noexcept_or_copy(  //Элементы перед новым
//          tmp.begin(), m_data.begin(), offset);
//      move_if_noexcept_or_copy(  //Элементы после нового
//          tmp.begin() + offset + 1, m_data.begin() + offset, m_size -
//          offset);
//      destroy_and_swap(m_data, m_size, tmp);
//    }
//  }
//  ++m_size;
//  return m_data.begin() + offset;  //Теперь по этому адресу - новый элемент
//}
//
// template <class Ty>
// typename vector<Ty>::iterator vector<Ty>::Erase(const_iterator it) {
//  iterator target{m_data.begin() + (it - m_data.begin())};
//  shift_left(  //Сдвигаем элементы влево
//      target, m_data.begin() + m_size);
//  PopBack();  //Допустимо, т.к. элементы сдвинуты, последний пожлежит
//  очистке return target;  //Теперь по этому адресу расположен элемент,
//  следующий за
//                  //удалённым
//}
// template <class Ty>
// void vector<Ty>::move_if_noexcept_or_copy_buf(raw_buffer& dst,
//                                              raw_buffer& src,
//                                              size_type count) {
//  move_if_noexcept_or_copy(dst.begin(), src.begin(), count);
//}
//
// template <class Ty>
// void vector<Ty>::move_if_noexcept_or_copy(Ty* dst, Ty* src, size_type
// count)
// {
//  if constexpr (std::is_nothrow_move_constructible_v<Ty>) {  // strong
//  guarantee
//    std::uninitialized_move_n(
//        src, count, dst);  //Оптимизировано для trivially copyable-типов
//  } else {
//    std::uninitialized_copy_n(src, count, dst);
//  }
//}
//
// template <class Ty>
// template <typename... Types>
// Ty& vector<Ty>::emplace_back_helper(Types&&... args) {
//  Ty* elem_ptr;
//  if (m_size < Capacity()) {  //Не изменяем my_size, чтобы не оставить
//  вектор в
//                              //несогласованном состоянии
//    elem_ptr = construct_in_place(
//        std::addressof(m_data[m_size]),
//        std::forward<Types>(args)...);  //при выбросе исключения
//  } else {
//    raw_buffer tmp(calculate_new_capacity(  //Вычисляем требуемую емкость
//        Capacity(), m_size + 1));
//    elem_ptr = new (std::addressof(tmp[m_size])) Ty(std::forward<Types>(
//        args)...);  //Сначала конструируем новый объект, т.к. в
//        EmplaceBack()
//    move_if_noexcept_or_copy_buf(  //мог быть передан один из элементов
//    текущего
//                                   //вектора
//        tmp, m_data, m_size);
//    destroy_and_swap(m_data, m_size, tmp);
//  }
//  return *elem_ptr;
//}
//
// template <class Ty>
// template <typename... Types>
// Ty* vector<Ty>::construct_in_place(void* place, Types&&... args) {
//  return new (place) Ty(std::forward<Types>(args)...);
//}
//
// template <class Ty>
// void vector<Ty>::destroy_helper(raw_buffer& buf, size_type count) {
//  std::destroy_n(buf.begin(), count);
//}
//
// template <class Ty>
// void vector<Ty>::destroy_and_swap(raw_buffer& old_buf,
//                                  size_type count,
//                                  raw_buffer& new_buf) {
//  destroy_helper(old_buf, count);
//  old_buf.Swap(new_buf);
//}
//
}  // namespace ktl
#endif