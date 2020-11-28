#pragma once
#include <ntddk.h>

namespace ktl {
struct NtSuccess {
  constexpr bool operator()(NTSTATUS status) const noexcept {
    return NT_SUCCESS(status);
  }
};

constexpr bool StatusSuccess(NTSTATUS status) {
  return NtSuccess{}(status);
}

template <NTSTATUS expected>
struct NtStatusEqual {
  constexpr bool operator()(NTSTATUS status) const noexcept {
    return status == expected;
  }
};
}