#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <vector>
namespace ktl {
using std::vector;
}
#else
#include <algorithm.hpp>
#include <allocator.hpp>
#include <basic_types.h>
#include <iterator.hpp>
#include <type_traits.hpp>
#include <utility.hpp>
namespace ktl {
namespace mm::details {
template <class Allocator>
class raw_memory {
 private:
  using AlTy = Allocator;
  using AlTyTraits = allocator_traits<AlTy>;
  using value_type = typename AlTyTraits::value_type;
  using pointer = typename AlTyTraits::pointer;
  using const_pointer = typename AlTyTraits::const_pointer;
  using reference = typename AlTyTraits::reference;
  using const_reference = typename AlTyTraits::const_reference;
  using size_type = typename AlTyTraits::size_type;

 public:
  raw_memory(Allocator& alc) noexcept
      : m_alc{addressof(alc)}, m_buf{nullptr}, m_capacity{0} {}
  raw_memory(Allocator& alc, size_type object_count) : raw_memory(alc) {
    m_buf = allocate(*m_alc, object_count);
    if (m_buf) {
      m_capacity = object_count;
    }
  }

  raw_memory(const raw_memory&) = delete;
  raw_memory(raw_memory&& other) noexcept
      : m_buf{exchange(other.m_buf, nullptr)},
        m_capacity{exchange(other.m_capacity, 0)} {}

  raw_memory& operator=(const raw_memory&) = delete;
  raw_memory& operator=(raw_memory&& other) noexcept {
    if (this != addressof(other)) {
      deallocate(*m_alc, _buf, m_capacity);
      m_buf = exchange(other.m_buf, nullptr);
      m_capacity = exchange(other.m_capacity, 0);
    }
    return *this;
  }
  ~raw_memory() { deallocate(*m_alc, m_buf, m_capacity); }

  void swap(raw_memory& other) noexcept {
    swap(m_buf, other.m_buf);
    swap(m_capacity, other.m_capacity);
  }
  size_type capacity() const noexcept { return m_capacity; }

  pointer begin() noexcept { return m_buf; }
  pointer end() noexcept { return m_buf + m_capacity; }
  const_pointer begin() const noexcept { return m_buf; }
  const_pointer end() const noexcept { return m_buf + m_capacity; }

  reference operator[](size_type idx) noexcept { return m_buf[idx]; }
  const_reference operator[](size_type idx) const noexcept {
    return m_buf[idx];
  }

  bool is_null() const noexcept { return !m_buf; }
  explicit operator bool() const noexcept { return m_buf; }

 private:
  static pointer allocate(Allocator& alc, size_type object_count) {
    return AlTyTraits::allocate(alc, object_count);
  }
  static void deallocate(Allocator& alc, pointer buf, size_type object_count) {
    if constexpr (AlTyTraits::enable_delete_null::value) {
      AlTyTraits::deallocate(alc, buf, object_count);
    } else {
      if (buf) {
        AlTyTraits::deallocate(alc, buf, object_count);
      }
    }
  }

 private:
  Allocator* m_alc;
  pointer m_buf;
  size_type m_capacity;
};

template <class size_type>
struct size_info {
  size_info() = default;

  size_info(const size_info&) = default;
  size_info& operator=(const size_info&) = default;

  size_info(size_info&& other) : size { exchange(other.size, 0) }
#ifdef KTL_NO_EXCEPTIONS
  , not_corrupted { exchange(other.not_corrupted, false) }
#endif
  {}
  size_info& operator=(size_info&& other) {
    if (this != addressof(other)) {
      size = exchange(other.m_size, 0);
#ifdef KTL_NO_EXCEPTIONS
      not_corrupted = exchange(other.m_not_corrupted, false);
#endif
    }
    return this;
  }

  void swap(size_info& other) noexcept {
    swap(size, other.size);
#ifdef KTL_NO_EXCEPTIONS
    swap(not_corrupted, other.not_corrupted);
#endif
  }

#ifdef KTL_NO_EXCEPTIONS
  bool not_corrupted() const noexcept { return not_corrupted; }
  bool not_corrupted{true};
#endif

  size_type size{0};
};
}  // namespace mm::details

#ifndef KTL_NO_EXCEPTIONS
template <class Ty, class Allocator = basic_paged_allocator<Ty> >
#else
template <class Ty,
          class Verifier,
          class Allocator = basic_paged_allocator<Ty> >
#endif
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

#ifdef KTL_NO_EXCEPTIONS
  using verifier_type = Verifier;
#endif

  using iterator = pointer;
  using const_iterator = const_pointer;

 private:
  static constexpr size_type MIN_CAPACITY{1}, GROWTH_COEF{2};

 public:
#ifndef KTL_NO_EXCEPTIONS
  vector() noexcept(is_nothrow_default_constructible_v<Allocator>)
      : m_data(m_alc), m_not_corrupted{true} {}
#else
  vector() noexcept(is_nothrow_default_constructible_v<Allocator>&&
                        is_nothrow_default_constructible_v<Verifier>)
      : m_data(m_alc) {}
#endif

  vector(size_type object_count) noexcept(
      noexcept(vector()) && is_nothrow_default_constructible_v<Ty>)
      : m_data{mm::details::raw_memory(m_alc, object_count)} {
    if (!m_data.is_null()) {
#ifndef KTL_NO_EXCEPTIONS
      uninitialized_default_construct_n(m_data.begin(), object_count);
      m_size = object_count;
#else
      m_size_info.not_corrupted =
          uninitialized_default_construct_n(m_vf, m_data.begin(), object_count);
      if (m_size_into.not_corrupted()) {
        m_size_into.size = object_count;
      }
#endif
    }
  }

  vector(const vector& other);
  vector(vector&& other) = default;  // noexcept
  ~vector();

  vector& operator=(const vector& other);
  vector& operator=(vector&& other) = default;  // noexcept

  void swap(vector& other) noexcept {
    m_data.swap(other.m_data);
    m_size_info.swap(other.m_size_info);
  }

  bool reserve(size_type object_count) {
    if (object_count > capacity()) {
      raw_memory tmp(n);
      // move_if_noexcept_or_copy_buf(tmp, m_data, m_size);
      // destroy_and_swap(m_data, m_size, tmp);
    }
  }
  void Resize(size_type n);

  bool push_back(const Ty& elem) { return emplace_back(elem); }
  bool push_back(Ty&& elem) { return emplace_back(move(elem)); }

  void pop_back() {  // UB if empty
    destroy_at(addressof(back()));
    --m_size_info.size;
  }

  template <typename... Types>
  bool emplace_back(Types&&... args);

  size_type size() const noexcept { return m_size_info.size; }
  size_type capacity() const noexcept { return m_data.capacity(); }

  const_reference operator[](size_type i) const { return m_data[i]; }
  reference operator[](size_type i) { return m_data[i]; }

  iterator begin() noexcept { return m_data.begin(); }
  iterator end() noexcept { return m_data.begin() + m_size_info.size; }

  const_iterator begin() const noexcept { return cbegin(); }
  const_iterator end() const noexcept { return cend(); }

  const_iterator cbegin() const noexcept { return m_data.begin(); }
  const_iterator cend() const noexcept {
    return m_data.begin() + m_size_info.size;
  }

  reference front() { return return m_data[0]; }
  reference back() { return return m_data[size() - 1]; }

  const_reference front() const { return return m_data[0]; }
  const_reference back() { return return m_data[size() - 1]; }

  // Вставляет элемент перед pos
  // Возвращает итератор на вставленный элемент
  iterator Insert(const_iterator pos, const Ty& elem);
  iterator Insert(const_iterator pos, Ty&& elem);

  // Конструирует элемент по заданным аргументам конструктора перед pos
  // Возвращает итератор на вставленный элемент
  template <typename... Types>
  iterator Emplace(const_iterator it, Types&&... args);

  // Удаляет элемент на позиции pos
  // Возвращает итератор на элемент, следующий за удалённым
  iterator Erase(const_iterator it);

 private:
  class raw_memory;
  static size_type calculate_new_capacity(size_type current_capacity,
                                          size_type summary_object_cnt) {
    if (current_capacity >= summary_object_cnt) {
      return current_capacity;
    }
    return current_capacity
               ? (max)(MIN_CAPACITY, summary_object_cnt)
               : (max)(current_capacity * GROWTH_COEF, summary_object_cnt);
  }
  static void move_if_noexcept_or_copy_buf(raw_memory& dst,
                                           raw_memory& src,
                                           size_type count);
  static void move_if_noexcept_or_copy(Ty* dst, Ty* src, size_type count);

  template <typename... Types>
  Ty& emplace_back_helper(Types&&... args);  //Не изменяет размер
  template <typename... Types>
  static Ty* construct_in_place(void* place, Types&&... args);
  static void destroy_helper(raw_memory& buf, size_type count);
  static void destroy_and_swap(
      raw_memory& old_buf,
      size_type count,
      raw_memory& new_buf);  //Очищает старый буфер и заменяет его на новый

 private:
  Allocator m_alc;
#ifdef KTL_NO_EXCEPTIONS
  Verifier m_vf;
#endif
  mm::details::raw_memory<Allocator> m_data;
  mm::details::size_info<size_type> m_size_info;
};  // namespace ktl

template <class Ty>
vector<Ty>::vector(const vector& other)
    : m_data(other.m_size), m_size{other.m_size} {
  std::uninitialized_copy_n(other.m_data.begin(), other.m_size, m_data.begin());
}

template <class Ty>
vector<Ty>::~vector() {
  destroy_helper(m_data, m_size);
}

template <class Ty>
vector<Ty>& vector<Ty>::operator=(const vector& other) {
  if (this != std::addressof(other)) {
    if (other.m_size >
        Capacity()) {  //Если размер копируемого вектора больше, чем емкость
                       //текущего - не реаллоцируем уже имеющиеся элементы
      vector tmp(other);  // Copy-and-Swap
      Swap(tmp);
    } else {
      const size_type min_size{std::min(m_size, other.m_size)};
      std::copy(  //Для примитивных типов std::copy использует memmove()
          other.m_data.begin(), other.m_data.begin() + min_size,
          m_data.begin());
      if (min_size == other.m_size) {  //Текущий вектор больше копируемого
        std::destroy(m_data.begin() + min_size, m_data.begin() + m_size);
      } else {
        std::uninitialized_copy_n(other.m_data.begin() + min_size,
                                  other.m_size - min_size,
                                  m_data.begin() + min_size);
      }
    }
    m_size = other.m_size;
  }
  return *this;
}

template <class Ty>
vector<Ty>& vector<Ty>::operator=(vector&& other) noexcept {
  if (this != std::addressof(other)) {
    vector tmp(std::move(other));
    Swap(tmp);
  }
  return *this;
}

template <class Ty>
void vector<Ty>::Reserve(size_type n) {
  if (n > Capacity()) {
    raw_memory tmp(n);
    move_if_noexcept_or_copy_buf(tmp, m_data, m_size);
    destroy_and_swap(m_data, m_size, tmp);
  }
}

template <class Ty>
void vector<Ty>::Resize(size_type n) {
  Reserve(n);
  if (n < m_size) {
    std::destroy(m_data.begin() + n, m_data.begin() + m_size);
  } else {
    std::uninitialized_default_construct(  //Конструирование объектов по
                                           //умолчанию
        m_data.begin() + m_size, m_data.begin() + n);
  }
  m_size = n;
}

template <class Ty>
template <typename... Types>
Ty& vector<Ty>::EmplaceBack(Types&&... args) {
  Ty& elem_ref{emplace_back_helper(std::forward<Types>(args)...)};
  ++m_size;
  return elem_ref;
}

template <class Ty>
typename vector<Ty>::iterator vector<Ty>::Insert(const_iterator pos,
                                                 const Ty& elem) {
  return Emplace(pos, elem);
}

template <class Ty>
typename vector<Ty>::iterator vector<Ty>::Insert(const_iterator pos,
                                                 Ty&& elem) {
  return Emplace(pos, std::move(elem));
}

template <class Ty>
template <typename... Types>
typename vector<Ty>::iterator vector<Ty>::Emplace(const_iterator it,
                                                  Types&&... args) {
  auto offset{it - m_data.begin()};
  if (offset == m_size) {  //Смещение от начала
    emplace_back_helper(std::forward<Types>(args)...);
  } else {
    if (m_size < Capacity()) {
      emplace_back_helper(std::move_if_noexcept(
          m_data[m_size - 1]));  //Конструируем собственный последний элемент на
                                 // 1 позицию правее
      shift_right(
          m_data.begin() + offset,
          m_data.begin() + m_size  //Значение m_size пока остается прежним
      );
      construct_in_place(std::addressof(m_data[offset]),
                         std::forward<Types>(args)...);
    } else {
      raw_memory tmp(calculate_new_capacity(  //Вычисляем требуемую емкость
          Capacity(), m_size + 1));
      construct_in_place(std::addressof(tmp[offset]),
                         std::forward<Types>(args)...);  //Конструируем элемент
      move_if_noexcept_or_copy(  //Элементы перед новым
          tmp.begin(), m_data.begin(), offset);
      move_if_noexcept_or_copy(  //Элементы после нового
          tmp.begin() + offset + 1, m_data.begin() + offset, m_size - offset);
      destroy_and_swap(m_data, m_size, tmp);
    }
  }
  ++m_size;
  return m_data.begin() + offset;  //Теперь по этому адресу - новый элемент
}

template <class Ty>
typename vector<Ty>::iterator vector<Ty>::Erase(const_iterator it) {
  iterator target{m_data.begin() + (it - m_data.begin())};
  shift_left(  //Сдвигаем элементы влево
      target, m_data.begin() + m_size);
  PopBack();  //Допустимо, т.к. элементы сдвинуты, последний пожлежит очистке
  return target;  //Теперь по этому адресу расположен элемент, следующий за
                  //удалённым
}
template <class Ty>
void vector<Ty>::move_if_noexcept_or_copy_buf(raw_memory& dst,
                                              raw_memory& src,
                                              size_type count) {
  move_if_noexcept_or_copy(dst.begin(), src.begin(), count);
}

template <class Ty>
void vector<Ty>::move_if_noexcept_or_copy(Ty* dst, Ty* src, size_type count) {
  if constexpr (std::is_nothrow_move_constructible_v<Ty>) {  // strong guarantee
    std::uninitialized_move_n(
        src, count, dst);  //Оптимизировано для trivially copyable-типов
  } else {
    std::uninitialized_copy_n(src, count, dst);
  }
}

template <class Ty>
template <typename... Types>
Ty& vector<Ty>::emplace_back_helper(Types&&... args) {
  Ty* elem_ptr;
  if (m_size < Capacity()) {  //Не изменяем my_size, чтобы не оставить вектор в
                              //несогласованном состоянии
    elem_ptr = construct_in_place(
        std::addressof(m_data[m_size]),
        std::forward<Types>(args)...);  //при выбросе исключения
  } else {
    raw_memory tmp(calculate_new_capacity(  //Вычисляем требуемую емкость
        Capacity(), m_size + 1));
    elem_ptr = new (std::addressof(tmp[m_size])) Ty(std::forward<Types>(
        args)...);  //Сначала конструируем новый объект, т.к. в EmplaceBack()
    move_if_noexcept_or_copy_buf(  //мог быть передан один из элементов текущего
                                   //вектора
        tmp, m_data, m_size);
    destroy_and_swap(m_data, m_size, tmp);
  }
  return *elem_ptr;
}

template <class Ty>
template <typename... Types>
Ty* vector<Ty>::construct_in_place(void* place, Types&&... args) {
  return new (place) Ty(std::forward<Types>(args)...);
}

template <class Ty>
void vector<Ty>::destroy_helper(raw_memory& buf, size_type count) {
  std::destroy_n(buf.begin(), count);
}

template <class Ty>
void vector<Ty>::destroy_and_swap(raw_memory& old_buf,
                                  size_type count,
                                  raw_memory& new_buf) {
  destroy_helper(old_buf, count);
  old_buf.Swap(new_buf);
}

template <class Ty>
vector<Ty>::raw_memory::raw_memory(size_type n)
    : m_buf{allocate(n)}, m_capacity{n} {}

template <class Ty>
vector<Ty>::raw_memory::raw_memory(raw_memory&& other) noexcept
    : m_buf{std::exchange(other.m_buf, nullptr)},
      m_capacity{std::exchange(other.m_capacity, 0)} {}

template <class Ty>
typename vector<Ty>::raw_memory& vector<Ty>::raw_memory::operator=(
    raw_memory&& other) noexcept {
  if (this != std::addressof(other)) {
    raw_memory tmp(std::move(other));
    Swap(tmp);  // Move-and-Swap
  }
  return *this;
}

template <class Ty>
vector<Ty>::raw_memory::~raw_memory() {
  deallocate(m_buf);
}

}  // namespace ktl
#endif