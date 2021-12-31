# Redistribution and use is allowed under the MIT license.
# Copyright (c) 2021 Dmitry Bolshakov. All rights reserved.

macro(KTL_ADD_TEST_IMPL TEST_NAME RUNNER_NAME)
	cmake_parse_arguments(KTL "" "" "TEST_RUNNER" ${ARGN})
	set(TARGET_NAME ${TEST_NAME}_test)

	wdk_add_library(
		${TARGET_NAME} STATIC
			EXTENDED_CPP_FEATURES
			WINVER ${TARGET_WINVER}
			${KTL_UNPARSED_ARGUMENTS}
	)
	add_library(tests::${TEST_NAME} ALIAS ${TARGET_NAME})

	target_include_directories(
		${TARGET_NAME} PUBLIC 
			${KTL_DIR}
			${CMAKE_CURRENT_SOURCE_DIR}
	)

	target_compile_definitions(
		${TARGET_NAME} PRIVATE
			"$<$<CONFIG:Debug>:KTL_CPP_DBG>"
			KTL_NO_CXX_STANDARD_LIBRARY 
			${FMT_COMPILE_DEFINITIONS}
	)
	target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)
	target_compile_options(
		${TARGET_NAME} PRIVATE
			${BASIC_COMPILE_OPTIONS}
			$<$<CONFIG:Release>:${RELEASE_COMPILE_OPTIONS}>
			$<$<CONFIG:Release>:${LTO_COMPILE_OPTIONS}>
	)

	target_link_options(
		${TARGET_NAME} PRIVATE
			$<$<CONFIG:Release>:${RELEASE_LINK_OPTIONS}>
			$<$<CONFIG:Release>:${LTO_LINK_OPTIONS}>
	)
	target_link_libraries(
		${TARGET_NAME} PUBLIC 
			basic_runtime
			cpp_runtime
			cpp_runtime
			${RUNNER_NAME}
	)
endmacro()

function(ktl_add_test test_name)
	KTL_ADD_TEST_IMPL(${test_name} "" ${ARGN})
endfunction()


function(ktl_add_test_with_runner test_name)
	KTL_ADD_TEST_IMPL(${test_name} tests::runner ${ARGN})
endfunction()


