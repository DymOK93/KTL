cmake_minimum_required (VERSION 3.0)
project ("Kernel Tempalte Library")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_INCLUDE_DIR "${KTL_DIR}/include")

set(
	KTL_RUNTIME_HEADER_FILES
		"algorithm.hpp"
		"allocator.hpp"
		"atomic.hpp"
		"chrono.hpp"
		"driver_base.h"
		"exception.hpp"
		"functional.hpp"
		"iterator.hpp"
		"limits.hpp"
		"memory.hpp"
		"memory_tools.hpp"
		"memory_type_traits.hpp"
		"mutex.hpp"
		"new_delete.h"
		"nt_status.h"
		"object_base.h"
		"raii.hpp"
		"smart_pointer.hpp"
		"static_pipeline.hpp"
		"string.hpp"
		"string_algorithms.hpp"
		"string_base.hpp"
		"type_traits.hpp"
		"unicode_string_view.hpp"
		"utility.hpp"
		"vector.hpp"
)

set(TARGET_LIB cpp_interface)

add_library(${TARGET_LIB} INTERFACE)
target_compile_definitions(
	${TARGET_LIB}  INTERFACE 
		_CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES=0  # Для исправления ошибки с затемнением параметров шаблона в старых хедерах CRT
)
target_include_directories(
	${TARGET_LIB} 
		INTERFACE ${KTL_INCLUDE_DIR}
)
target_link_libraries(
	${TARGET_LIB} 
		INTERFACE basic_runtime_interface
)