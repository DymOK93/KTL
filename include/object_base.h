#pragma once
#include <ntddk.h>
#include <utility.hpp>


namespace ktl {
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
}  // namespace ktl