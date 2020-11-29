#pragma once
namespace ktl {
namespace dirty {  //Требуется для взаимодействия с WinAPI
template <class Ty>
constexpr const Ty* remove_const_from_pointer(const Ty* const ptr) {
  return const_cast<const Ty*>(ptr);
}
template <class Ty>
constexpr Ty* remove_const_from_value(const Ty* ptr) {
  return const_cast<Ty*>(ptr);
}
template <class Ty>
constexpr Ty* remove_const(const Ty* const ptr) {
  return const_cast<Ty*>(ptr);
}
}  // namespace dirty
}  // namespace ktl
