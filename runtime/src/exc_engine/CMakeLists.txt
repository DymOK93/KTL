cmake_minimum_required (VERSION 3.0)
project ("CXX Exception Engine")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_EXC_ENGINE_SOURCE_DIR "${KTL_RUNTIME_SOURCE_DIR}/exc_engine")

set(
	KTL_EXC_ENGINE_SOURCE_FILES
		"gs_failure.cpp"
		"member_ptr.cpp"
		"terminate.cpp"
)

set(TARGET_LIB exc_engine)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_subdirectory(x86)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_subdirectory(x64)
endif()

wdk_add_library(
	${TARGET_LIB} OBJECT
		WINVER ${TARGET_WINVER}
		${KTL_EXC_ENGINE_HEADER_FILES}
		${KTL_EXC_ENGINE_SOURCE_FILES}
		EXTENDED_CPP_FEATURES
)
target_include_directories(
	${TARGET_LIB} 
		PUBLIC ${KTL_EXC_ENGINE_SOURCE_DIR}
)
target_compile_options(
	${TARGET_LIB} PRIVATE
		${BASIC_COMPILE_OPTIONS}
		$<$<CONFIG:Release>:${RELEASE_COMPILE_OPTIONS}>
		$<$<CONFIG:Release>:${LTO_COMPILE_OPTIONS}>
)
target_compile_definitions(
	${TARGET_LIB} PRIVATE
		$<$<CONFIG:Debug>:KTL_RUNTIME_DBG>
		KTL_NO_CXX_STANDARD_LIBRARY
)
target_link_options(
	${TARGET_LIB} PRIVATE
		$<$<CONFIG:Release>:${RELEASE_LINK_OPTIONS}>
		$<$<CONFIG:Release>:${LTO_LINK_OPTIONS}>
)
target_link_libraries(
	${TARGET_LIB} 
		PUBLIC basic_runtime_interface
		PUBLIC exc_engine_interface
)

