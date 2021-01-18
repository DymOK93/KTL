cmake_minimum_required (VERSION 3.0)
project ("Kernel Template Library")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(WDK REQUIRED)

set(KTL_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(KTL_RUNTIME_DIR "${KTL_DIR}/runtime")
set(KTL_INCLUDE_DIR "${KTL_DIR}/include")
set(KTL_SOURCE_DIR "${KTL_DIR}/src")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

set(
	KTL_SOURCE_FILES
		"${KTL_SOURCE_DIR}/new_delete.cpp"
)

set(TARGET_LIB cpp_runtime)

add_subdirectory(runtime)

wdk_add_library(
	${TARGET_LIB} STATIC 
		${KTL_HEADER_FILES}
		${KTL_SOURCE_FILES}
)
target_include_directories(
	${TARGET_LIB} PUBLIC
		${KTL_INCLUDE_DIR}
)
target_compile_definitions(
	${TARGET_LIB} PRIVATE
		KTL_RUNTIME_DBG
		KTL_NO_CXX_STANDARD_LIBRARY
)
target_link_libraries(${TARGET_LIB} basic_runtime)