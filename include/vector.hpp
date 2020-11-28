#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <vector>
namespace ktl {
	using std::vector;
}
#else
#include <type_traits.hpp>
#include <iterator>

template <class Ty>
class Vector {
 public:
  Vector() = default;
  Vector(size_t n);
  Vector(const Vector& other);
  Vector(Vector&& other) noexcept;
  ~Vector();

  Vector& operator=(const Vector& other);
  Vector& operator=(Vector&& other) noexcept;

  void Swap(Vector& other) noexcept;

  void Reserve(size_t n);
  void Resize(size_t n);

  void PushBack(const Ty& elem);
  void PushBack(Ty&& elem);
  void PopBack();

  template <typename... Types>
  Ty& EmplaceBack(Types&&... args);

  size_t Size() const noexcept;
  size_t Capacity() const noexcept;

  const Ty& operator[](size_t i) const;
  Ty& operator[](size_t i);

  // В данной части задачи реализуйте дополнительно эти функции:
  using iterator = Ty*;
  using const_iterator = const Ty*;

  iterator begin() noexcept;
  iterator end() noexcept;

  const_iterator begin() const noexcept;
  const_iterator end() const noexcept;

  // Тут должна быть такая же реализация, как и для константных версий begin/end
  const_iterator cbegin() const noexcept;
  const_iterator cend() const noexcept;

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
  class MemoryBuffer;
  static size_t calculate_new_capacity(size_t current_capacity,
                                       size_t summary_object_cnt);
  static void move_if_noexcept_or_copy_buf(MemoryBuffer& dst,
                                           MemoryBuffer& src,
                                           size_t count);
  static void move_if_noexcept_or_copy(Ty* dst, Ty* src, size_t count);

  template <typename... Types>
  Ty& emplace_back_helper(Types&&... args);  //Не изменяет размер
  template <typename... Types>
  static Ty* construct_in_place(void* place, Types&&... args);
  static void destroy_helper(MemoryBuffer& buf, size_t count);
  static void destroy_and_swap(
      MemoryBuffer& old_buf,
      size_t count,
      MemoryBuffer& new_buf);  //Очищает старый буфер и заменяет его на новый
 private:
  inline static constexpr size_t min_capacity{1}, growth_coef{2};

 private:
  class MemoryBuffer {
   public:
    MemoryBuffer() = default;
    MemoryBuffer(size_t n);
    MemoryBuffer(const MemoryBuffer&) = delete;
    MemoryBuffer(MemoryBuffer&& other) noexcept;
    MemoryBuffer& operator=(const MemoryBuffer&) = delete;
    MemoryBuffer& operator=(MemoryBuffer&& other) noexcept;
    ~MemoryBuffer();

    void Swap(MemoryBuffer& other) noexcept;
    size_t Capacity() const noexcept;

    Ty* begin() noexcept;
    Ty* end() noexcept;
    const Ty* begin() const noexcept;
    const Ty* end() const noexcept;

    Ty& operator[](size_t idx) noexcept;
    const Ty& operator[](size_t idx) const noexcept;

   private:
    static Ty* allocate(size_t object_count);
    void deallocate(Ty* buf);

   private:
    Ty* m_buf{nullptr};
    size_t m_capacity{0};
  };
  MemoryBuffer m_data;
  size_t m_size{0};
};

template <class Ty>
Vector<Ty>::Vector(size_t n) : m_data{MemoryBuffer(n)}, m_size{n} {
  std::uninitialized_default_construct_n(m_data.begin(), n);
}

template <class Ty>
Vector<Ty>::Vector(const Vector& other)
    : m_data(other.m_size), m_size{other.m_size} {
  std::uninitialized_copy_n(other.m_data.begin(), other.m_size, m_data.begin());
}

template <class Ty>
Vector<Ty>::Vector(Vector&& other) noexcept
    : m_data(std::move(other.m_data)), m_size{std::exchange(other.m_size, 0)} {}

template <class Ty>
Vector<Ty>::~Vector() {
  destroy_helper(m_data, m_size);
}

template <class Ty>
Vector<Ty>& Vector<Ty>::operator=(const Vector& other) {
  if (this != std::addressof(other)) {
    if (other.m_size >
        Capacity()) {  //Если размер копируемого вектора больше, чем емкость
                       //текущего - не реаллоцируем уже имеющиеся элементы
      Vector tmp(other);  // Copy-and-Swap
      Swap(tmp);
    } else {
      const size_t min_size{std::min(m_size, other.m_size)};
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
Vector<Ty>& Vector<Ty>::operator=(Vector&& other) noexcept {
  if (this != std::addressof(other)) {
    Vector tmp(std::move(other));
    Swap(tmp);
  }
  return *this;
}

template <class Ty>
void Vector<Ty>::Swap(Vector& other) noexcept {
  m_data.Swap(other.m_data);
  std::swap(m_size, other.m_size);
}

template <class Ty>
void Vector<Ty>::Reserve(size_t n) {
  if (n > Capacity()) {
    MemoryBuffer tmp(n);
    move_if_noexcept_or_copy_buf(tmp, m_data, m_size);
    destroy_and_swap(m_data, m_size, tmp);
  }
}

template <class Ty>
void Vector<Ty>::Resize(size_t n) {
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
void Vector<Ty>::PushBack(const Ty& elem) {
  EmplaceBack(elem);
}

template <class Ty>
void Vector<Ty>::PushBack(Ty&& elem) {
  EmplaceBack(std::move(elem));
}

template <class Ty>
void Vector<Ty>::PopBack() {
  std::destroy_at(m_data.begin() + m_size - 1);
  --m_size;  //Не забываем обновить размер вектора
}

template <class Ty>
template <typename... Types>
Ty& Vector<Ty>::EmplaceBack(Types&&... args) {
  Ty& elem_ref{emplace_back_helper(std::forward<Types>(args)...)};
  ++m_size;
  return elem_ref;
}

template <class Ty>
size_t Vector<Ty>::Size() const noexcept {
  return m_size;
}

template <class Ty>
size_t Vector<Ty>::Capacity() const noexcept {
  return m_data.Capacity();
}

template <class Ty>
const Ty& Vector<Ty>::operator[](size_t i) const {
  return m_data[i];
}

template <class Ty>
Ty& Vector<Ty>::operator[](size_t i) {
  return m_data[i];
}

template <class Ty>
typename Vector<Ty>::iterator Vector<Ty>::begin() noexcept {
  return m_data.begin();
}

template <class Ty>
typename Vector<Ty>::iterator Vector<Ty>::end() noexcept {
  return m_data.begin() + m_size;
}

template <class Ty>
typename Vector<Ty>::const_iterator Vector<Ty>::begin() const noexcept {
  return m_data.begin();
}

template <class Ty>
typename Vector<Ty>::const_iterator Vector<Ty>::end() const noexcept {
  return m_data.begin() + m_size;
}

template <class Ty>
typename Vector<Ty>::const_iterator Vector<Ty>::cbegin() const noexcept {
  return begin();
}

template <class Ty>
typename Vector<Ty>::const_iterator Vector<Ty>::cend() const noexcept {
  return end();
}

template <class Ty>
typename Vector<Ty>::iterator Vector<Ty>::Insert(const_iterator pos,
                                                 const Ty& elem) {
  return Emplace(pos, elem);
}

template <class Ty>
typename Vector<Ty>::iterator Vector<Ty>::Insert(const_iterator pos,
                                                 Ty&& elem) {
  return Emplace(pos, std::move(elem));
}

template <class Ty>
template <typename... Types>
typename Vector<Ty>::iterator Vector<Ty>::Emplace(const_iterator it,
                                                  Types&&... args) {
  auto offset{it - m_data.begin()};
  if (offset == m_size) {  //Смещение от начала
    emplace_back_helper(std::forward<Types>(args)...);
  } else {
    if (m_size < Capacity()) {
      emplace_back_helper(std::move_if_noexcept(
          m_data[m_size - 1]));  //Конструируем собственный последний элемент на
                                 //1 позицию правее
      shift_right(
          m_data.begin() + offset,
          m_data.begin() + m_size  //Значение m_size пока остается прежним
      );
      construct_in_place(std::addressof(m_data[offset]),
                         std::forward<Types>(args)...);
    } else {
      MemoryBuffer tmp(calculate_new_capacity(  //Вычисляем требуемую емкость
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
typename Vector<Ty>::iterator Vector<Ty>::Erase(const_iterator it) {
  iterator target{m_data.begin() + (it - m_data.begin())};
  shift_left(  //Сдвигаем элементы влево
      target, m_data.begin() + m_size);
  PopBack();  //Допустимо, т.к. элементы сдвинуты, последний пожлежит очистке
  return target;  //Теперь по этому адресу расположен элемент, следующий за
                  //удалённым
}

template <class Ty>
size_t Vector<Ty>::calculate_new_capacity(size_t current_capacity,
                                          size_t summary_object_cnt) {
  if (current_capacity >= summary_object_cnt) {
    return current_capacity;
  }
  return current_capacity
             ? std::max(min_capacity, summary_object_cnt)
             : std::max(current_capacity * growth_coef, summary_object_cnt);
}

template <class Ty>
void Vector<Ty>::move_if_noexcept_or_copy_buf(MemoryBuffer& dst,
                                              MemoryBuffer& src,
                                              size_t count) {
  move_if_noexcept_or_copy(dst.begin(), src.begin(), count);
}

template <class Ty>
void Vector<Ty>::move_if_noexcept_or_copy(Ty* dst, Ty* src, size_t count) {
  if constexpr (std::is_nothrow_move_constructible_v<Ty>) {  // strong guarantee
    std::uninitialized_move_n(
        src, count, dst);  //Оптимизировано для trivially copyable-типов
  } else {
    std::uninitialized_copy_n(src, count, dst);
  }
}

template <class Ty>
template <typename... Types>
Ty& Vector<Ty>::emplace_back_helper(Types&&... args) {
  Ty* elem_ptr;
  if (m_size < Capacity()) {  //Не изменяем my_size, чтобы не оставить вектор в
                              //несогласованном состоянии
    elem_ptr = construct_in_place(
        std::addressof(m_data[m_size]),
        std::forward<Types>(args)...);  //при выбросе исключения
  } else {
    MemoryBuffer tmp(calculate_new_capacity(  //Вычисляем требуемую емкость
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
Ty* Vector<Ty>::construct_in_place(void* place, Types&&... args) {
  return new (place) Ty(std::forward<Types>(args)...);
}

template <class Ty>
void Vector<Ty>::destroy_helper(MemoryBuffer& buf, size_t count) {
  std::destroy_n(buf.begin(), count);
}

template <class Ty>
void Vector<Ty>::destroy_and_swap(MemoryBuffer& old_buf,
                                  size_t count,
                                  MemoryBuffer& new_buf) {
  destroy_helper(old_buf, count);
  old_buf.Swap(new_buf);
}

template <class Ty>
Vector<Ty>::MemoryBuffer::MemoryBuffer(size_t n)
    : m_buf{allocate(n)}, m_capacity{n} {}

template <class Ty>
Vector<Ty>::MemoryBuffer::MemoryBuffer(MemoryBuffer&& other) noexcept
    : m_buf{std::exchange(other.m_buf, nullptr)},
      m_capacity{std::exchange(other.m_capacity, 0)} {}

template <class Ty>
typename Vector<Ty>::MemoryBuffer& Vector<Ty>::MemoryBuffer::operator=(
    MemoryBuffer&& other) noexcept {
  if (this != std::addressof(other)) {
    MemoryBuffer tmp(std::move(other));
    Swap(tmp);  // Move-and-Swap
  }
  return *this;
}

template <class Ty>
Vector<Ty>::MemoryBuffer::~MemoryBuffer() {
  deallocate(m_buf);
}

template <class Ty>
void Vector<Ty>::MemoryBuffer::Swap(MemoryBuffer& other) noexcept {
  std::swap(m_buf, other.m_buf);
  std::swap(m_capacity, other.m_capacity);
}

template <class Ty>
size_t Vector<Ty>::MemoryBuffer::Capacity() const noexcept {
  return m_capacity;
}

template <class Ty>
Ty* Vector<Ty>::MemoryBuffer::begin() noexcept {
  return m_buf;
}

template <class Ty>
Ty* Vector<Ty>::MemoryBuffer::end() noexcept {
  return m_buf + m_capacity;
}

template <class Ty>
const Ty* Vector<Ty>::MemoryBuffer::begin() const noexcept {
  return m_buf;
}

template <class Ty>
const Ty* Vector<Ty>::MemoryBuffer::end() const noexcept {
  return m_buf + m_capacity;
}

template <class Ty>
Ty& Vector<Ty>::MemoryBuffer::operator[](size_t idx) noexcept {
  return m_buf[idx];
}

template <class Ty>
const Ty& Vector<Ty>::MemoryBuffer::operator[](size_t idx) const noexcept {
  return m_buf[idx];
}

template <class Ty>
Ty* Vector<Ty>::MemoryBuffer::allocate(size_t object_count) {
  return static_cast<Ty*>(operator new[](object_count * sizeof(Ty)));
}

template <class Ty>
void Vector<Ty>::MemoryBuffer::deallocate(Ty* buf) {
  operator delete[](buf);
}
#endif