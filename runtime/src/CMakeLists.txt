cmake_minimum_required (VERSION 3.0)
project ("KTL RTL Modules")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_RUNTIME_SOURCE_DIR "${KTL_RUNTIME_DIR}/src")

set(
	KTL_RUNTIME_SOURCE_FILES
		"assert.cpp"
		"basic_runtime.cpp"
		"bugcheck.cpp"
		"chrono_impl.cpp"
		"exception.cpp"
		"exception_base.cpp"
		"exception_impl.cpp"
		"floating_point.cpp" 
		"irql.cpp"
		"heap.cpp"
		"object_management.cpp"
		"placement_new.cpp"
		"preload_initializer.cpp"
		"type_info.cpp"
)
set(
	KTL_MINIFILTER_RUNTIME_SOURCE_FILES
		"minifilter.cpp"
)

set(RUNTIME_LIB basic_runtime)
set(MINIFILTER_LIB minifilter_runtime)

add_subdirectory(exc_engine)

wdk_add_library(
	${RUNTIME_LIB} STATIC 
		WINVER ${TARGET_WINVER}
		${KTL_RUNTIME_HEADER_FILES}
		${KTL_RUNTIME_SOURCE_FILES}
		EXTENDED_CPP_FEATURES
)
target_compile_options(
	${RUNTIME_LIB} PRIVATE
		${BASIC_COMPILE_OPTIONS}
		$<$<CONFIG:Release>:${RELEASE_COMPILE_OPTIONS}>
		$<$<CONFIG:Release>:${LTO_COMPILE_OPTIONS}>
)
target_compile_definitions(
	${RUNTIME_LIB} PRIVATE
		KTL_RUNTIME_DBG
		KTL_NO_CXX_STANDARD_LIBRARY
		_CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES=0  # Для исправления ошибки с затемнением параметров шаблона в старых хедерах CRT
)
target_link_options(
	${RUNTIME_LIB} PRIVATE
		$<$<CONFIG:Release>:${RELEASE_LINK_OPTIONS}>
		$<$<CONFIG:Release>:${LTO_LINK_OPTIONS}>
)
target_link_libraries(
	${RUNTIME_LIB} 
		PUBLIC basic_runtime_interface
		PUBLIC exc_engine
)
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	target_link_libraries(
		${RUNTIME_LIB} 
			PUBLIC exc_x86
	)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
	target_link_libraries(
		${RUNTIME_LIB} 
			PUBLIC exc_x64
	)
endif()
target_link_options(
	${RUNTIME_LIB} 
		PRIVATE "/WHOLEARCHIVE"
)

wdk_add_library(
	${MINIFILTER_LIB} STATIC 
		WINVER ${TARGET_WINVER}
		${KTL_RUNTIME_HEADER_FILES}	
		${KTL_MINIFILTER_RUNTIME_SOURCE_FILES}
)
target_compile_options(
	${MINIFILTER_LIB} PRIVATE
		${BASIC_COMPILE_OPTIONS}
		$<$<CONFIG:Release>:${RELEASE_COMPILE_OPTIONS}>
		$<$<CONFIG:Release>:${LTO_COMPILE_OPTIONS}>
)
target_compile_definitions(
	${MINIFILTER_LIB} PRIVATE
		$<$<CONFIG:Debug>:KTL_RUNTIME_DBG>
		KTL_NO_CXX_STANDARD_LIBRARY
		_CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES=0  # Для исправления ошибки с затемнением параметров шаблона в старых хедерах CRT
)
target_link_options(
	${MINIFILTER_LIB} PRIVATE
		$<$<CONFIG:Release>:${RELEASE_LINK_OPTIONS}>
		$<$<CONFIG:Release>:${LTO_LINK_OPTIONS}>
)
target_link_libraries(
	${MINIFILTER_LIB} 
		PUBLIC minifilter_runtime_interface
)
