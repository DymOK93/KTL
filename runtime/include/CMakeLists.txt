cmake_minimum_required (VERSION 3.0)
project ("KTL Runtime Library")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_RUNTIME_INCLUDE_DIR "${KTL_RUNTIME_DIR}/include")

set(
	KTL_RUNTIME_HEADER_FILES
		"algorithm_impl.hpp"
		"basic_runtime.hpp"
		"basic_types.hpp"
		"bugcheck.hpp"
		"char_traits_impl.hpp"
		"chrono_impl.hpp"
		"cookie.hpp"
		"crt_assert.hpp"
		"crt_attributes.hpp"
		"crt_runtime.hpp"
		"flag_set.hpp"
		"functional_impl.hpp"
		"intrinsic.hpp"
		"heap.hpp"
		"irql.hpp"
		"limits_impl.hpp"
		"memory_type_traits_impl.hpp"
		"minifilter.hpp"
		"object_management.hpp"
		"placement_new.hpp"
		"preload_initializer.hpp"
		"type_info.hpp"
		"type_traits_impl.hpp"
		"utility_impl.hpp"
)

set(RUNTIME_LIB basic_runtime_interface)

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
