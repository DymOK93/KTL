cmake_minimum_required (VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(WDK REQUIRED)

set(
	KTL_RUNTIME_SOURCE_FILES
	"${KTL_RUNTIME_SOURCE_DIR}/basic_runtime.cpp"
)

wdk_add_library(
	basic_runtime STATIC 
		${KTL_RUNTIME_HEADER_FILES}
		${KTL_RUNTIME_SOURCE_FILES}
)

target_include_directories(
	basic_runtime PUBLIC
		${KTL_RUNTIME_INCLUDE_DIR}
)