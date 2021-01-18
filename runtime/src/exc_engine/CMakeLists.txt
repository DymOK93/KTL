cmake_minimum_required (VERSION 3.0)
project ("KTL Exception Engine")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_EXC_ENGINE_DIR "${KTL_RUNTIME_SOURCE_DIR}/exc_engine")

set(
	KTL_EXC_ENGINE_HEADER_FILES
		"exc_engine_interface"
		"funclet_array.h"
		"stack_walker.h"
)

set(
	KTL_EXC_ENGINE_SOURCE_FILES
		"engine.cpp"
)

set(TARGET_LIB exc_engine)

add_subdirectory(x64)

wdk_add_library(
	${TARGET_LIB} OBJECT
		${KTL_EXC_ENGINE_HEADER_FILES}
		${KTL_EXC_ENGINE_SOURCE_FILES}
)
target_include_directories(
	${TARGET_LIB} 
		PUBLIC ${KTL_EXC_ENGINE_DIR}
)
target_compile_definitions(
	${TARGET_LIB} PRIVATE
		KTL_RUNTIME_DBG
		KTL_NO_CXX_STANDARD_LIBRARY
)
target_link_libraries(
	${TARGET_LIB} 
		PUBLIC basic_runtime_interface
		PUBLIC exc_x64
)
add_link_options(
	"/WHOLEARCHIVE"
)