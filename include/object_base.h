#pragma once
#include <ntddk.h>
#include <utility.hpp>

// deprecated

namespace ktl {
class ObjectBase {
 public:
  NTSTATUS GetStatus() const noexcept { return m_status; }
  bool Fail() const noexcept { return m_status != STATUS_SUCCESS; }

 protected:
  NTSTATUS set_status(NTSTATUS status) const {
    return exchange(m_status,
                    status);  //������������� ����� ������ � ���������� ������
  }

 private:
  mutable NTSTATUS m_status{STATUS_SUCCESS};
};
}  // namespace ktl