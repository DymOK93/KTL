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
#include <type_traits.hpp>
#include <utility.hpp>

namespace ktl {
namespace mm::details {
template <class Allocator>
class raw_buffer {
 public:
  using allocator_type = Allocator;
  using allocator_traits_type = allocator_traits<allocator_type>;

  using value_type = typename allocator_traits_type::value_type;
  using pointer = typename allocator_traits_type::pointer;
  using const_pointer = typename allocator_traits_type::const_pointer;
  using reference = typename allocator_traits_type::reference;
  using const_reference = typename allocator_traits_type::const_reference;
  using size_type = typename allocator_traits_type::size_type;

 public:
  raw_buffer() noexcept(is_nothrow_default_constructible_v<allocator_type>) =
      default;

  raw_buffer(const allocator_type& alloc) noexcept(
      is_nothrow_copy_constructible_v<allocator_type>)
      : m_buffer{one_then_variadic_args{}, alloc} {}

  raw_buffer(allocator_type&& alloc) noexcept(
      is_nothrow_move_constructible_v<allocator_type>)
      : m_buffer{one_then_variadic_args{}, move(alloc)} {}

  template <class Alloc = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Alloc>, int> = 0>
  raw_buffer(Alloc&& alloc = Alloc{}) noexcept(
      is_nothrow_constructible_v<allocator_type, Alloc>)
      : m_buffer{one_then_variadic_args{}, forward<Alloc>(alloc)} {}

  template <class Alloc = allocator_type,
            enable_if_t<is_constructible_v<allocator_type, Alloc>, int> = 0>
  raw_buffer(size_type object_count, Alloc&& alloc = Alloc{})
      : m_buffer{one_then_variadic_args{}, forward<Alloc>(alloc)} {
    auto& alc{m_buffer.get_first()};
    m_buffer.get_second() = allocate_helper(alc, object_count);
  }

  raw_buffer(const raw_buffer&) = delete;

  raw_buffer(raw_buffer&& other) noexcept(
      is_nothrow_move_constructible_v<allocator_type>)
      : m_buffer{one_then_variadic_args{}, move(get_allocator()),
                 exchange(other.m_buffer.get_second(), nullptr)},
        m_capacity{exchange(other.m_capacity, 0)} {}

  raw_buffer& operator=(const raw_buffer&) = delete;

  raw_buffer& operator=(raw_buffer&& other) noexcept(
      is_nothrow_move_assignable_v<allocator_type>) {
    if (addressof(other) != this) {
      deallocate_helper(m_buffer.get_first(), m_buffer.get_second(),
                        m_capacity);
      get_allocator() = move(get_allocator());
      other.m_buffer.get_second() =
          exchange(other.m_buffer.get_second(), nullptr);
      m_capacity = exchange(other.m_capacity, 0);
    }
    return *this;
  }

  ~raw_buffer() {
    deallocate_helper(get_allocator(), m_buffer.get_second(), m_capacity);
  }

  void swap(raw_buffer& other) noexcept(
      is_nothrow_swappable_v<allocator_type>) {
    swap(m_buffer, other.m_buffer);
    swap(m_capacity, other.m_capacity);
  }

  size_type capacity() const noexcept { return m_capacity; }

  pointer data() noexcept { return m_buffer.get_second(); }
  const_pointer data() const noexcept { return m_buffer.get_second(); }

  pointer begin() noexcept { return data(); }
  pointer end() noexcept { return data() + capacity(); }
  const_pointer begin() const noexcept { return cbegin(); }
  const_pointer end() const noexcept { return cend(); }
  const_pointer cbegin() const noexcept { return data(); }
  const_pointer cend() const noexcept { return data() + capacity(); }

  reference operator[](size_type idx) noexcept { return data()[idx]; }

  const_reference operator[](size_type idx) const noexcept {
    return data()[idx];
  }

  explicit operator bool() const noexcept { return static_cast<bool>(data()); }

 private:
  allocator_type& get_allocator() noexcept { return m_buffer.get_first(); }

  static pointer allocate_helper(allocator_type& alc, size_type object_count) {
    return allocator_traits_type::allocate(alc, object_count);
  }

  static void deallocate_helper(allocator_type& alc,
                                pointer buf,
                                size_type object_count) {
    if constexpr (allocator_traits_type::enable_delete_null::value) {
      allocator_traits_type::deallocate(alc, buf, object_count);
    } else {
      if (buf) {
        allocator_traits_type::deallocate(alc, buf, object_count);
      }
    }
  }

 private:
  compressed_pair<allocator_type, pointer> m_buffer{};
  size_type m_capacity{0};
};
}  // namespace mm::details

template <class Ty, class Allocator = basic_paged_allocator<Ty> >
class vector {
 public:
  using value_type = Ty;

  using allocator_type = Allocator;
  using allocator_traits_type = allocator_traits<allocator_type>;

  using pointer = Ty*;
  using const_pointer = const Ty*;
  using reference = Ty&;
  using const_reference = const Ty&;
  using size_type = typename allocator_traits_type::size_type;

  using iterator = pointer;
  using const_iterator = const_pointer;

 private:
  using memory_buffer_type = mm::details::raw_buffer<allocator_type>;

 public:
  static_assert(is_same_v<Ty, typename memory_buffer_type::value_type>,
                "Uncompatible allocator");

 private:
  static constexpr size_type MIN_CAPACITY{1}, GROWTH_MULTIPLIER{2};

 public:
  vector() noexcept(is_nothrow_default_constructible_v<memory_buffer_type>) =
      default;

  explicit vector(const allocator_type& alloc) noexcept(
      is_nothrow_constructible_v<memory_buffer_type, const allocator_type&>)
      : m_buffer{alloc} {}

  explicit vector(allocator_type&& alloc) noexcept(
      is_nothrow_constructible_v<memory_buffer_type, allocator_type&&>)
      : m_buffer{alloc} {}

  template <class Alloc = allocator_type,
            enable_if_t<is_constructible_v<memory_buffer_type, Alloc>, int> = 1>
  vector(Alloc&& alloc = Alloc{}) noexcept(
      is_nothrow_constructible_v<memory_buffer_type, Alloc>)
      : m_buffer{forward<Alloc>(alloc)} {}

  template <class Alloc = allocator_type>
  vector(size_type object_count, Alloc&& alloc = Alloc{}) noexcept(
      is_nothrow_constructible_v<memory_buffer_type, Alloc>&&
          is_nothrow_default_constructible_v<Ty>)
      : m_buffer{object_count, forward<Alloc>(alloc)}, m_size{object_count} {
    uninitialized_default_construct_n(m_buffer.begin(), object_count);
  }

  template <class Alloc = allocator_type>
  vector(size_type object_count,
         const Ty& value,
         Alloc&& alloc =
             Alloc{}) noexcept(is_nothrow_constructible_v<memory_buffer_type,
                                                          Alloc>&&
                                   is_nothrow_default_constructible_v<Ty>)
      : m_buffer{object_count, forward<Alloc>(alloc)}, m_size{object_count} {
    uninitialized_fill_n(m_buffer.begin(), object_count, value);
  }

  // vector(const vector& other);
  vector(vector&& other) noexcept(
      is_nothrow_move_constructible_v<memory_buffer_type>)
      : m_buffer{move(other.m_buffer)}, m_size{exchange(other.m_size, 0)} {}

  ~vector() { destroy_n(m_buffer.begin(), m_size); }

  /*vector& operator=(const vector& other);
  vector& operator=(vector&& other);*/

  void swap(vector& other) noexcept(
      is_nothrow_swappable_v<memory_buffer_type>) {
    m_buffer.swap(other.m_buffer);
    ::swap(m_size, other.m_size);
  }

  // bool reserve(size_type object_count) {
  //   if (object_count > capacity()) {
  //     raw_buffer tmp(n);
  //     // move_if_noexcept_or_copy_buf(tmp, m_data, m_size);
  //     // destroy_and_swap(m_data, m_size, tmp);
  //   }
  // }
  // void Resize(size_type n);

  // bool push_back(const Ty& elem) { return emplace_back(elem); }
  // bool push_back(Ty&& elem) { return emplace_back(move(elem)); }

  void pop_back() {  // UB if empty
    destroy_at(addressof(back()));
    --m_size;
  }

  // template <typename... Types>
  // bool emplace_back(Types&&... args);

  size_type size() const noexcept { return m_size; }
  bool empty() const noexcept { return size() == 0; }
  size_type capacity() const noexcept { return m_buffer.capacity(); }

  pointer data() noexcept { return m_buffer.data(); }
  const_pointer data() const noexcept { m_buffer.data(); }

  reference operator[](size_type idx) noexcept {
    assert_with_msg(idx < size(), L"index is out of range");
    return data()[idx];
  }

  const_reference operator[](size_type idx) const noexcept {
    assert_with_msg(idx < size(), L"index is out of range");
    return data()[idx];
  }

  reference at(size_type idx) {
    return at_index_verified(m_buffer, size(), idx);
  }

  const_reference at(size_type idx) const {
    return at_index_verified(m_buffer, size(), idx);
  }

  iterator begin() noexcept { return m_buffer.begin(); }
  iterator end() noexcept { return begin() + size(); }

  const_iterator begin() const noexcept { return cbegin(); }
  const_iterator end() const noexcept { return cend(); }

  const_iterator cbegin() const noexcept { return m_buffer.cbegin(); }
  const_iterator cend() noexcept { return cbegin() + size(); }

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

  // // Вставляет элемент перед pos
  // // Возвращает итератор на вставленный элемент
  // iterator Insert(const_iterator pos, const Ty& elem);
  // iterator Insert(const_iterator pos, Ty&& elem);

  // // Конструирует элемент по заданным аргументам конструктора перед pos
  // // Возвращает итератор на вставленный элемент
  // template <typename... Types>
  // iterator Emplace(const_iterator it, Types&&... args);

  // // Удаляет элемент на позиции pos
  // // Возвращает итератор на элемент, следующий за удалённым
  // iterator Erase(const_iterator it);

  // private:
  // static size_type calculate_new_capacity(size_type current_capacity,
  //                                         size_type summary_object_cnt) {
  //   if (current_capacity >= summary_object_cnt) {
  //     return current_capacity;
  //   }
  //   return current_capacity
  //              ? (max)(MIN_CAPACITY, summary_object_cnt)
  //              : (max)(current_capacity * GROWTH_COEF, summary_object_cnt);
  // }
  // static void move_if_noexcept_or_copy_buf(raw_buffer& dst,
  //                                          raw_buffer& src,
  //                                          size_type count);
  // static void move_if_noexcept_or_copy(Ty* dst, Ty* src, size_type count);

  // template <typename... Types>
  // Ty& emplace_back_helper(Types&&... args);  //Не изменяет размер
  // template <typename... Types>
  // static Ty* construct_in_place(void* place, Types&&... args);
  // static void destroy_helper(raw_buffer& buf, size_type count);
  // static void destroy_and_swap(
  //     raw_buffer& old_buf,
  //     size_type count,
  //     raw_buffer& new_buf);  //Очищает старый буфер и заменяет его на новый

 private:
  template <class Buffer>
  static decltype(auto) at_index_verified(Buffer& buf,
                                          size_type size,
                                          size_type idx) {
    throw_exception_if_not<out_of_range>(idx < size, L"index is out of range",
                                         constexpr_message_tag{});
    return buf[idx];
  }

 private:
  mm::details::raw_buffer<Allocator> m_buffer;
  size_type m_size{0};
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
//          m_data[m_size - 1]));  //Конструируем собственный последний элемент
//          на
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
//          tmp.begin() + offset + 1, m_data.begin() + offset, m_size - offset);
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
//  PopBack();  //Допустимо, т.к. элементы сдвинуты, последний пожлежит очистке
//  return target;  //Теперь по этому адресу расположен элемент, следующий за
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
// void vector<Ty>::move_if_noexcept_or_copy(Ty* dst, Ty* src, size_type count)
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
//  if (m_size < Capacity()) {  //Не изменяем my_size, чтобы не оставить вектор
//  в
//                              //несогласованном состоянии
//    elem_ptr = construct_in_place(
//        std::addressof(m_data[m_size]),
//        std::forward<Types>(args)...);  //при выбросе исключения
//  } else {
//    raw_buffer tmp(calculate_new_capacity(  //Вычисляем требуемую емкость
//        Capacity(), m_size + 1));
//    elem_ptr = new (std::addressof(tmp[m_size])) Ty(std::forward<Types>(
//        args)...);  //Сначала конструируем новый объект, т.к. в EmplaceBack()
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