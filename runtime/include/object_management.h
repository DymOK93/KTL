#pragma once
#include <basic_types.h>
#include <crt_attributes.h>

#include <ntddk.h>

namespace ktl::crt {
using handler_t = void(CRTCALL*)(void);

void CRTCALL invoke_constructors() noexcept;
void CRTCALL invoke_destructors() noexcept;
}  // namespace ktl::crt
