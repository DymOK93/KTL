cmake_minimum_required (VERSION 3.0)
project ("CXX Exception Engine X64")
enable_language(CXX ASM_MASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_EXC_X64_DIR "${KTL_EXC_ENGINE_SOURCE_DIR}/x64")

set(
	KTL_EXC_X64_HEADER_FILES
		"catch.h"
		"cpu_context.h"
		"throw.h"
)
set(
	KTL_EXC_X64_SOURCE_FILES
		"catch.cpp"
		"cookie.cpp"
		"throw.cpp"
		"frame_handler3.cpp"
		"frame_handler4.cpp"
		"unwind_handler"
)
set(
	KTL_EXC_X64_ASM_FILES
		"capture.asm"
		"cookie.asm"
)

set(TARGET_LIB exc_x64)

wdk_add_library(
	${TARGET_LIB} OBJECT
		WINVER ${TARGET_WINVER}
		${KTL_EXC_X64_HEADER_FILES}
		${KTL_EXC_X64_SOURCE_FILES}
		${KTL_EXC_X64_ASM_FILES}
		EXTENDED_CPP_FEATURES
)
target_include_directories(
	${TARGET_LIB} 
		PUBLIC ${KTL_EXC_X64_DIR}
)
target_compile_options(
	${TARGET_LIB} PRIVATE
		${BASIC_COMPILE_OPTIONS}
		$<$<CONFIG:Release>:${RELEASE_COMPILE_OPTIONS}>	# Unfortunately, __GSHandlerCheck_EH* 
														# and some other parts of the exception engine 
														# can't be compiled with LTO enabled
)
target_compile_definitions(
	${TARGET_LIB} PRIVATE
		$<$<CONFIG:Debug>:KTL_RUNTIME_DBG>
		KTL_NO_CXX_STANDARD_LIBRARY
)
target_link_options(
	${TARGET_LIB} PRIVATE
		$<$<CONFIG:Release>:${RELEASE_LINK_OPTIONS}>
)
target_link_libraries(
	${TARGET_LIB} 
		PRIVATE basic_runtime_interface
		PRIVATE exc_engine_interface
)
