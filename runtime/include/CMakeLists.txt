cmake_minimum_required (VERSION 3.0)
project ("KTL Runtime Library")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_RUNTIME_INCLUDE_DIR "${KTL_RUNTIME_DIR}/include")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

set(
	KTL_RUNTIME_HEADER_FILES
		"basic_runtime.h"
		"basic_types.h"
		"crt_attributes.h"
		"crt_runtime.h"
		"exception_base.h"
		"exc_engine"
		"heap.h"
		"irql.h"
		"placement_new.h"
)

set(TARGET_LIB basic_runtime_interface)

add_library(${TARGET_LIB} INTERFACE)
target_include_directories(
	${TARGET_LIB} 
		INTERFACE ${KTL_RUNTIME_INCLUDE_DIR} 
)