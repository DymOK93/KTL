cmake_minimum_required (VERSION 3.0)
project ("Formatting Library")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_FMT_DIR "${KTL_MODULES_DIR}/fmt")

set(TARGET_LIB kfmt)

set(
	KTL_FMT_HEADER_FILES
		"compile.h"
		"core.h"
		"format.h"
		"format-inl.h"
		"xchar.h"
)

add_library(${TARGET_LIB} INTERFACE)
target_include_directories(
	${TARGET_LIB} 
		INTERFACE ${KTL_FMT_DIR}
)
target_link_libraries(
	${TARGET_LIB} 
		INTERFACE basic_runtime_interface
		INTERFACE cpp_runtime_interface
)
