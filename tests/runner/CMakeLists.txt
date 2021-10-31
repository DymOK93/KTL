wdk_add_library(
	test_runner STATIC
		EXTENDED_CPP_FEATURES
		WINVER ${TARGET_WINVER}
			"container_traits.hpp"
			"test_runner.hpp"
			"test_runner.cpp"
)
add_library(tests::runner ALIAS test_runner)

target_compile_definitions(
	test_runner PRIVATE
		"$<$<CONFIG:Debug>:KTL_CPP_DBG>"
		KTL_NO_CXX_STANDARD_LIBRARY 
		${FMT_COMPILE_DEFINITIONS}
)

target_compile_features(test_runner PRIVATE cxx_std_17)

target_compile_options(
	test_runner PRIVATE
		${BASIC_COMPILE_OPTIONS}
		$<$<CONFIG:Release>:${RELEASE_COMPILE_OPTIONS}>
		$<$<CONFIG:Release>:${LTO_COMPILE_OPTIONS}>
)

target_link_options(
	test_runner PRIVATE
		$<$<CONFIG:Release>:${RELEASE_LINK_OPTIONS}>
		$<$<CONFIG:Release>:${LTO_LINK_OPTIONS}>
)

target_include_directories(
	test_runner PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
		${KTL_DIR}
)

target_link_libraries(
	test_runner PUBLIC
		basic_runtime
		cpp_runtime
)

