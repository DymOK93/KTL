cmake_minimum_required (VERSION 3.8)
project ("Driver 9")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(WDK REQUIRED)

set(KTL_RUNTIME_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(KTL_RUNTIME_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

foreach("${CMAKE_CURRENT_SOURCE_DIR}/*.h")

endforeach()

set(
	KTL_RUNTIME_HEADER_FILES
		"${KTL_RUNTIME_INCLUDE_DIR}/basic_runtime.h"
		"${KTL_RUNTIME_INCLUDE_DIR}/basic_types.h"
		"${KTL_RUNTIME_INCLUDE_DIR}/crt_runtime.h"
		"${KTL_RUNTIME_INCLUDE_DIR}/exception.h"
		"${KTL_RUNTIME_INCLUDE_DIR}/heap.h"
)

set(
	KTL_RUNTIME_SOURCE_FILES
	"${KTL_RUNTIME_SOURCE_DIR}/basic_runtime.cpp"
)

wdk_add_library(
	KtlRuntime STATIC 
		${KTL_RUNTIME_HEADER_FILES}
		${KTL_RUNTIME_SOURCE_FILES}
)
