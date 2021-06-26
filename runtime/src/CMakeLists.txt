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
		"exception_base.cpp"
		"exception_impl.cpp"
		"irql.cpp"
		"heap.cpp"
		"placement_new.cpp"
		"preload_initializer.cpp"
		"type_info.cpp"
)

set(TARGET_LIB basic_runtime)

add_subdirectory(exc_engine)

wdk_add_library(
	${TARGET_LIB} STATIC 
		${KTL_RUNTIME_HEADER_FILES}
		${KTL_RUNTIME_SOURCE_FILES}
		EXTENDED_CPP_FEATURES
)
target_compile_definitions(
	${TARGET_LIB} PRIVATE
		KTL_RUNTIME_DBG
		KTL_NO_CXX_STANDARD_LIBRARY
		_CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES=0  # Для исправления ошибки с затемнением параметров шаблона в старых хедерах CRT
)

target_link_libraries(
	${TARGET_LIB} 
		PUBLIC cpp_interface
		PUBLIC basic_runtime_interface
		PUBLIC exc_engine
)
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	target_link_libraries(
		${TARGET_LIB} 
			PUBLIC exc_x86
	)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
	target_link_libraries(
		${TARGET_LIB} 
			PUBLIC exc_x64
	)
endif()

target_link_options(
	${TARGET_LIB} 
		PRIVATE "/WHOLEARCHIVE"
)

