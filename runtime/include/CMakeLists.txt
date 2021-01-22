cmake_minimum_required (VERSION 3.0)
project ("KTL Runtime Library")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_RUNTIME_INCLUDE_DIR "${KTL_RUNTIME_DIR}/include")

set(
	KTL_RUNTIME_HEADER_FILES
		"algorithm_impl.hpp"
		"assert.h"
		"basic_runtime.h"
		"basic_types.h"
		"bugcheck.h"
		"crt_attributes.h"
		"crt_runtime.h"
		"exception_base.h"
		"funtcional_impl.hpp"
		"heap.h"
		"irql.h"
		"limits_impl.hpp"
		"memory_type_traits_impl.hpp"
		"placement_new.h"
		"type_info.h"
		"type_traits_impl.hpp"
)

set(TARGET_LIB basic_runtime_interface)

add_subdirectory(exc_engine)

add_library(${TARGET_LIB} INTERFACE)
target_include_directories(
	${TARGET_LIB} 
		INTERFACE ${KTL_RUNTIME_INCLUDE_DIR}
)
target_compile_definitions(
	${TARGET_LIB} 
		INTERFACE KTL_ENABLE_EXTENDED_ALIGNED_STORAGE
)