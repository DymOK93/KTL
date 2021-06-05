cmake_minimum_required (VERSION 3.0)
project ("CXX Exception Engine")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_EXC_ENGINE_INCLUDE_DIR "${KTL_RUNTIME_INCLUDE_DIR}/exc_engine")

set(
	KTL_EXC_ENGINE_HEADER_FILES
		"cookie.h"
		"frame_handler.h"
		"member_ptr.h"
		"rva.hpp"
)

set(TARGET_LIB exc_engine_interface)

add_library(${TARGET_LIB} INTERFACE)
target_include_directories(
	${TARGET_LIB} 
		INTERFACE ${KTL_EXC_ENGINE_INCLUDE_DIR}
)
target_compile_definitions(
	${TARGET_LIB} 
		INTERFACE KTL_RUNTIME_DBG
		INTERFACE KTL_NO_CXX_STANDARD_LIBRARY
)