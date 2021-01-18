cmake_minimum_required (VERSION 3.0)
project ("KTL RTL Modules")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_RUNTIME_SOURCE_DIR "${KTL_RUNTIME_DIR}/src")

set(
	KTL_RUNTIME_SOURCE_FILES
		"basic_runtime.cpp"
		"exception_base.cpp"
		"irql.cpp"
		"heap.cpp"
		"placement_new.cpp"
)

set(TARGET_LIB basic_runtime)

add_subdirectory(exc_engine)

wdk_add_library(
	${TARGET_LIB} STATIC 
		${KTL_RUNTIME_HEADER_FILES}
		${KTL_RUNTIME_SOURCE_FILES}
)
target_compile_definitions(
	${TARGET_LIB} PRIVATE
		KTL_RUNTIME_DBG
		KTL_NO_CXX_STANDARD_LIBRARY
)
target_link_libraries(
	${TARGET_LIB} 
		PUBLIC basic_runtime_interface
		PRIVATE exc_engine
		PRIVATE exc_x64
)
target_link_options(
	${TARGET_LIB} 
		PRIVATE "/WHOLEARCHIVE"
)
