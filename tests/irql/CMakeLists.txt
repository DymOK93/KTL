wdk_add_library(
	irql_test STATIC
		EXTENDED_CPP_FEATURES
		WINVER ${TARGET_WINVER}
			"test.hpp"
			"test.cpp"
)
add_library(tests::irql ALIAS irql_test)

target_compile_definitions(
	irql_test PRIVATE
		"$<$<CONFIG:Debug>:KTL_CPP_DBG>"
		KTL_NO_CXX_STANDARD_LIBRARY 
		${FMT_COMPILE_DEFINITIONS}
)

target_compile_features(irql_test PRIVATE cxx_std_17)

target_compile_options(
	irql_test PRIVATE
		${BASIC_COMPILE_OPTIONS}
		$<$<CONFIG:Release>:${RELEASE_COMPILE_OPTIONS}>
		$<$<CONFIG:Release>:${LTO_COMPILE_OPTIONS}>
)

target_link_options(
	irql_test PRIVATE
		$<$<CONFIG:Release>:${RELEASE_LINK_OPTIONS}>
		$<$<CONFIG:Release>:${LTO_LINK_OPTIONS}>
)

target_include_directories(
	irql_test PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
	irql_test PUBLIC 
		basic_runtime
		cpp_runtime
		tests::runner
)


