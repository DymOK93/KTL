cmake_minimum_required (VERSION 3.0)
project ("CXX Exception Engine")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_EXC_ENGINE_SOURCE_DIR "${KTL_RUNTIME_SOURCE_DIR}/exc_engine")

set(
	KTL_EXC_ENGINE_SOURCE_FILES
		"member_ptr.cpp"
)

set(TARGET_LIB exc_engine)

wdk_add_library(
	${TARGET_LIB} OBJECT
		${KTL_EXC_ENGINE_HEADER_FILES}
		${KTL_EXC_ENGINE_SOURCE_FILES}
)
target_include_directories(
	${TARGET_LIB} 
		PUBLIC ${KTL_EXC_ENGINE_DIR}
)
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_subdirectory(x86)
	set(KTL_EXC_X64_DIR "${KTL_EXC_ENGINE_DIR}/x86")
	target_link_libraries(
		${TARGET_LIB} 
			PUBLIC basic_runtime_interface
			PRIVATE x86
	)
elseif(CMAKE_SIZEOF_VOID_P  EQUAL 8)
    add_subdirectory(x64)
	set(KTL_EXC_X64_DIR "${KTL_EXC_ENGINE_DIR}/x64")
	target_link_libraries(
		${TARGET_LIB} 
			PUBLIC exc_engine_interface
			PRIVATE x64
	)
endif()

target_compile_definitions(
	${TARGET_LIB} PRIVATE
		KTL_RUNTIME_DBG
		KTL_NO_CXX_STANDARD_LIBRARY
)
target_link_options(
	${TARGET_LIB} 
		PRIVATE "/WHOLEARCHIVE"
)
