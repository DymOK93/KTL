cmake_minimum_required (VERSION 3.0)
project ("KTL Runtime Library")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_RUNTIME_INCLUDE_DIR "${KTL_RUNTIME_DIR}/include")

set(
	KTL_RUNTIME_HEADER_FILES
		"algorithm_impl.hpp"
		"assert.h"
		"basic_runtime.h"
		"basic_types.h"
		"bugcheck.h"
		"char_traits_impl.hpp"
		"crt_attributes.h"
		"crt_runtime.h"
		"exception_base.h"
		"exception_impl.h"
		"functional_impl.hpp"
		"intrinsic.hpp"
		"heap.h"
		"irql.h"
		"limits_impl.hpp"
		"memory_type_traits_impl.hpp"
		"placement_new.h"
		"type_info.h"
		"type_traits_impl.hpp"
		"utility_impl.hpp"
)

set(TARGET_LIB basic_runtime_interface)

add_subdirectory(exc_engine)

add_library(${TARGET_LIB} INTERFACE)
target_include_directories(
	${TARGET_LIB} 
		INTERFACE ${KTL_RUNTIME_INCLUDE_DIR}
)
target_link_libraries(
	${TARGET_LIB} 
		INTERFACE exc_engine_interface
)
target_compile_definitions(
	${TARGET_LIB}  INTERFACE 
		KTL_ENABLE_EXTENDED_ALIGNED_STORAGE
		_CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES=0  # Для исправления ошибки с затемнением параметров шаблона в старых хедерах CRT
)