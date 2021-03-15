#pragma once
#include <basic_types.h>
#include <type_traits.hpp>

#include <ntddk.h>

namespace ktl {
struct device_info {
  unsigned long extension_size{0}, type{FILE_DEVICE_UNKNOWN},
      characteristics{0};
  bool exclusive{false};
};

// TODO: use ktl::pair
struct device_io_response {
  NTSTATUS status{STATUS_SUCCESS};
  uint32_t info{0};
};

template <class Ty>
Ty* get_from_device_extension(PDEVICE_OBJECT device_object) {
  return static_cast<add_pointer_t<remove_reference_t<Ty>>>(
      device_object->DeviceExtension);
}

template <class Ty>
Ty* get_from_device_extension(PDRIVER_OBJECT driver_object) {
  return get_from_device_extension<Ty>(driver_object->DeviceObject);
}
}  // namespace ktl