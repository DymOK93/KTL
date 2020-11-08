#pragma once

namespace winapi::kernel {
struct DeviceInfo {
  unsigned long extension_size{0},  //Дополнительная память, выделямое под
                                    //устройство (+ к sizeof(device))
      type{FILE_DEVICE_UNKNOWN},  //Тип устройства
      characteristics{0};  //Для некоторых специфических драйверов
  bool exclusive{false};  //Кол-во клиентов драйвера
};
}