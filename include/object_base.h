#pragma once
#include <ntddk.h>

#ifndef NO_CXX_STANDARD_LIBRARY
#include <utility>
#endif

namespace winapi::kernel {
class ObjectBase {
 public:
  NTSTATUS GetStatus() const noexcept { return m_status; }
  bool Fail() const noexcept { return m_status != STATUS_SUCCESS; }

 protected:
  NTSTATUS set_status(NTSTATUS status) {
    return std::exchange(
        m_status, status);  //”станавливает новый статус и возвращает старый
  }

 private:
  NTSTATUS m_status{STATUS_SUCCESS};
};
}  // namespace winapi::kernel