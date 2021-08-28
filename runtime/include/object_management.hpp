#pragma once
#include <basic_types.hpp>
#include <crt_attributes.hpp>

#include <ntddk.h>

namespace ktl::crt {
using handler_t = void(CRTCALL*)(void);

void CRTCALL invoke_constructors() noexcept;
void CRTCALL invoke_destructors() noexcept;
}  // namespace ktl::crt
