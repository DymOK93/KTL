#pragma once
#include <ntddk.h>

namespace ktl {
struct DeviceInfo {
  unsigned long extension_size{0},  //�������������� ������, ��������� ���
                                    //���������� (+ � sizeof(device))
      type{FILE_DEVICE_UNKNOWN},  //��� ����������
      characteristics{0};  //��� ��������� ������������� ���������
  bool exclusive{false};  //���-�� �������� ��������
};

template <class Driver>
Driver* GetDriverFromDeviceExtension(PDRIVER_OBJECT driver) {
  return static_cast<std::add_pointer_t<std::decay_t<Driver>>>(
      driver->DeviceObject->DeviceExtension);
}
}  // namespace ktl