wdk_add_library(
	fp_test STATIC
		EXTENDED_CPP_FEATURES
		WINVER ${TARGET_WINVER}
			"test.hpp"
			"test.cpp"
)
add_library(tests::floating_point ALIAS fp_test)

target_compile_definitions(
	fp_test PRIVATE
		"$<$<CONFIG:Debug>:KTL_CPP_DBG>"
		KTL_NO_CXX_STANDARD_LIBRARY 
		${FMT_COMPILE_DEFINITIONS}
)

target_compile_features(fp_test PRIVATE cxx_std_17)

target_compile_options(
	fp_test PRIVATE
		${BASIC_COMPILE_OPTIONS}
		$<$<CONFIG:Release>:${RELEASE_COMPILE_OPTIONS}>
		$<$<CONFIG:Release>:${LTO_COMPILE_OPTIONS}>
)

target_link_options(
	fp_test PRIVATE
		$<$<CONFIG:Release>:${RELEASE_LINK_OPTIONS}>
		$<$<CONFIG:Release>:${LTO_LINK_OPTIONS}>
)

target_include_directories(
	fp_test PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
	fp_test PUBLIC 
		basic_runtime
		cpp_runtime
		tests::runner
)


