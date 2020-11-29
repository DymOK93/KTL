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

template <class Driver>
Driver* GetDriverFromDeviceExtension(PDRIVER_OBJECT driver) {
  return static_cast<std::add_pointer_t<std::decay_t<Driver>>>(
      driver->DeviceObject->DeviceExtension);
}
}  // namespace ktl