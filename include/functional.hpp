#pragma once

#ifndef NO_CXX_STANDARD_LIBRARY
#include <functional>
namespace winapi::kernel {
using std::less;
using std::greater;
}  // namespace winapi::kernel
#else
namespace winapi::kernel {
template <class Ty = void>
struct less {
  constexpr bool operator()(const Ty& lhs, const Ty& rhs) const {
    return lhs < rhs;
  }
};
template <class Ty = void>
struct greater {
  constexpr bool operator()(const Ty& lhs, const Ty& rhs) const {
    return lhs > rhs;
  }
};

}  // namespace winapi::kernel
#endif