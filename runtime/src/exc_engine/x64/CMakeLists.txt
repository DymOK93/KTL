cmake_minimum_required (VERSION 3.0)
project ("Exception Engine x64")
enable_language(CXX ASM_MASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_EXC_ENGINE_X64_DIR ${KTL_EXC_ENGINE_DIR}/x64)

set(
	KTL_EXC_X64_HEADER_FILES
		"internal_typedefs.h"
)
set(
	KTL_EXC_X64_SOURCE_FILES
		"unwind_frame.cpp"
)
set(
	KTL_EXC_X64_ASM_FILES
		"execute_handler.asm"
		"invoke_funclet.asm"		
		"restore_context.asm"		
)

set(TARGET_LIB exc_x64)

wdk_add_library(
	${TARGET_LIB} OBJECT 
		${KTL_EXC_X64_HEADER_FILES}
		${KTL_EXC_X64_SOURCE_FILES}
		${KTL_EXC_X64_ASM_FILES}
)
target_include_directories(
	${TARGET_LIB} 
		PUBLIC ${KTL_EXC_ENGINE_X64_DIR}
)
target_compile_definitions(
	${TARGET_LIB} PRIVATE
		KTL_RUNTIME_DBG
		KTL_NO_CXX_STANDARD_LIBRARY
)
target_link_libraries(
	${TARGET_LIB} 
		PUBLIC basic_runtime_interface
)
