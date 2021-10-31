set(KTL_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(heap)
add_subdirectory(runner)

wdk_add_driver(
	ktl_test
		CUSTOM_ENTRY_POINT KtlDriverEntry
		EXTENDED_CPP_FEATURES
		WINVER ${TARGET_WINVER}
			"driver.cpp"
)

target_compile_definitions(
	ktl_test PRIVATE
		"$<$<CONFIG:Debug>:KTL_CPP_DBG>"
		KTL_NO_CXX_STANDARD_LIBRARY 
		${FMT_COMPILE_DEFINITIONS}
)

target_compile_features(ktl_test PUBLIC cxx_std_17)

target_compile_options(
	ktl_test PRIVATE
		${BASIC_COMPILE_OPTIONS}
		$<$<CONFIG:Release>:${RELEASE_COMPILE_OPTIONS}>
		$<$<CONFIG:Release>:${LTO_COMPILE_OPTIONS}>
)

target_link_options(
	ktl_test PRIVATE
		$<$<CONFIG:Release>:${RELEASE_LINK_OPTIONS}>
		$<$<CONFIG:Release>:${LTO_LINK_OPTIONS}>
)

target_include_directories(ktl_test PRIVATE ${KTL_DIR} ${KTL_TEST_DIR})

target_link_libraries(
	ktl_test
		basic_runtime 
		cpp_runtime
		minifilter_runtime

		tests::runner
		tests::heap
)

wdk_sign_driver(
	ktl_test
		${CERTIFICATE_NAME}
		CERTIFICATE_PATH ${CERTIFICATE_PATH}
		TIMESTAMP_SERVER http://timestamp.comodoca.com/authenticode
)
