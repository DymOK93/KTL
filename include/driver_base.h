#pragma once

namespace winapi::kernel {
struct DeviceInfo {
  unsigned long extension_size{0},  //�������������� ������, ��������� ���
                                    //���������� (+ � sizeof(device))
      type{FILE_DEVICE_UNKNOWN},  //��� ����������
      characteristics{0};  //��� ��������� ������������� ���������
  bool exclusive{false};  //���-�� �������� ��������
};
}