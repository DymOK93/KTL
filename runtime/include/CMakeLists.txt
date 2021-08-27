cmake_minimum_required (VERSION 3.0)
project ("KTL Runtime Library")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_RUNTIME_INCLUDE_DIR "${KTL_RUNTIME_DIR}/include")
set(KTL_MINIFILTER_RUNTIME_INCLUDE_DIR ${KTL_RUNTIME_INCLUDE_DIR})

set(
	KTL_RUNTIME_HEADER_FILES
		"algorithm_impl.hpp"
		"basic_runtime.h"
		"basic_types.h"
		"bugcheck.h"
		"char_traits_impl.hpp"
		"chrono_impl.h"
		"crt_assert.h"
		"crt_attributes.h"
		"crt_runtime.h"
		"flag_set.hpp"
		"functional_impl.hpp"
		"intrinsic.hpp"
		"heap.h"
		"irql.h"
		"limits_impl.hpp"
		"memory_type_traits_impl.hpp"
		"object_management.h"
		"placement_new.h"
		"preload_initializer.h"
		"type_info.h"
		"type_traits_impl.hpp"
		"utility_impl.hpp"
)

set(
	KTL_MINIFILTER_RUNTIME_HEADER_FILES
		"minifilter.h"
)

set(RUNTIME_LIB basic_runtime_interface)
set(MINIFILTER_LIB minifilter_runtime_interface)

add_subdirectory(exc_engine)

add_library(${RUNTIME_LIB} INTERFACE)
target_include_directories(
	${RUNTIME_LIB} 
		INTERFACE ${KTL_RUNTIME_INCLUDE_DIR}
)
target_link_libraries(
	${RUNTIME_LIB} 
		INTERFACE exc_engine_interface
)
target_compile_definitions(
	${RUNTIME_LIB} INTERFACE 
		KTL_ENABLE_EXTENDED_ALIGNED_STORAGE
		_CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES=0  # Для исправления ошибки с затемнением параметров шаблона в старых хедерах CRT
)

add_library(${MINIFILTER_LIB} INTERFACE)
target_include_directories(
	${MINIFILTER_LIB} 
		INTERFACE ${KTL_MINIFILTER_RUNTIME_INCLUDE_DIR}
)