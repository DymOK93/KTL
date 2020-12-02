#pragma once
#include <ntddk.h>

namespace ktl {
struct DeviceInfo {
  unsigned long extension_size{0},  //Дополнительная память, выделямое под
                                    //устройство (+ к sizeof(device))
      type{FILE_DEVICE_UNKNOWN},  //Тип устройства
      characteristics{0};  //Для некоторых специфических драйверов
  bool exclusive{false};  //Кол-во клиентов драйвера
};

struct DeviceIoResponse {
  NTSTATUS status{STATUS_SUCCESS};
  uint32_t info{0};
};

template <class Driver>
Driver* GetDriverFromDeviceExtension(PDEVICE_OBJECT device_object) {
  return static_cast<std::add_pointer_t<std::decay_t<Driver>>>(
      device_object->DeviceExtension);
}

template <class Driver>
Driver* GetDriverFromDeviceExtension(PDRIVER_OBJECT driver_object) {
  return GetDriverFromDeviceExtension<Driver>(driver_object->DeviceObject);
}
}  // namespace ktl