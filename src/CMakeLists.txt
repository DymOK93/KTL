cmake_minimum_required (VERSION 3.0)
project ("Kernel Template Library")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_SOURCE_DIR "${KTL_DIR}/src")

set(
	KTL_SOURCE_FILES
		"condition_variable.cpp"
		"exception.cpp"
		"literals.cpp"
		"new_delete.cpp"
)

set(TARGET_LIB cpp_runtime)

wdk_add_library(
	${TARGET_LIB} STATIC
		${KTL_HEADER_FILES}
		${KTL_SOURCE_FILES}
		EXTENDED_CPP_FEATURES
)
target_include_directories(
	${TARGET_LIB} PUBLIC
		${KTL_INCLUDE_DIR}
)
target_compile_definitions(
	${TARGET_LIB} PRIVATE
		KTL_CPP_DBG
		KTL_NO_CXX_STANDARD_LIBRARY
)
target_link_libraries(
	${TARGET_LIB} 
		PUBLIC cpp_interface
		PUBLIC basic_runtime_interface
		PUBLIC basic_runtime
)
