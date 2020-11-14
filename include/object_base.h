#pragma once
#include <ntddk.h>
#include <utility.hpp>


namespace winapi::kernel {
class ObjectBase {
 public:
  NTSTATUS GetStatus() const noexcept { return m_status; }
  bool Fail() const noexcept { return m_status != STATUS_SUCCESS; }

 protected:
  NTSTATUS set_status(NTSTATUS status) {
    return exchange(
        m_status, status);  //������������� ����� ������ � ���������� ������
  }

 private:
  NTSTATUS m_status{STATUS_SUCCESS};
};
}  // namespace winapi::kernel