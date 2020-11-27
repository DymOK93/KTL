#pragma once

#ifndef KTL_NO_CXX_STANDARD_LIBRARY
#include <vector>
namespace winapi::kernel {
	using std::vector;
}
#else
#include <type_traits.hpp>
void foo() {
  
}
#endif