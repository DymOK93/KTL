#pragma once
#include <ntddk.h>
#include <basic_types.h>
#include <type_traits.hpp>

namespace ktl {
struct DeviceInfo {
  unsigned long extension_size{0},  //�������������� ������, ��������� ���
                                    //���������� (+ � sizeof(device))
      type{FILE_DEVICE_UNKNOWN},  //��� ����������
      characteristics{0};  //��� ��������� ������������� ���������
  bool exclusive{false};  //���-�� �������� ��������
};

struct DeviceIoResponse {
  NTSTATUS status{STATUS_SUCCESS};
  uint32_t info{0};
};

template <class Driver>
Driver* GetDriverFromDeviceExtension(PDEVICE_OBJECT device_object) {
  return static_cast<add_pointer_t<remove_reference_t<Driver>>>(
      device_object->DeviceExtension);
}

template <class Driver>
Driver* GetDriverFromDeviceExtension(PDRIVER_OBJECT driver_object) {
  return GetDriverFromDeviceExtension<Driver>(driver_object->DeviceObject);
}
}  // namespace ktl