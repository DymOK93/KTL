cmake_minimum_required (VERSION 3.0)
project ("Kernel Template Library")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_INCLUDE_DIR "${KTL_DIR}/include")

set(
	KTL_HEADER_FILES
		"algorithm.hpp"
		"allocator.hpp"
		"assert.hpp"
		"atomic.hpp"
		"chrono.hpp"
		"condition_variable.h"
		"driver_base.h"
		"exception.h"
		"functional.hpp"
		"initializer_list.hpp"
		"intrusive_ptr.hpp"
		"iterator.hpp"
		"limits.hpp"
		"memory.hpp"
		"memory_tools.hpp"
		"memory_type_traits.hpp"
		"mutex.hpp"
		"new_delete.h"
		"smart_pointer.hpp"
		"static_pipeline.hpp"
		"string.hpp"
		"string_fwd.hpp"
		"string_view.hpp"
		"string_algorithm_impl.hpp"
		"string_algorithms_old.hpp"
		"type_traits.hpp"
		"unordered_container_impl.hpp"
		"unordered_map.hpp"
		"unordered_set.hpp"
		"utility.hpp"
		"vector.hpp"
)

set(TARGET_LIB cpp_interface)

add_library(${TARGET_LIB} INTERFACE)
target_compile_definitions(
	${TARGET_LIB}  INTERFACE 
		_CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES=0  
)
target_include_directories(
	${TARGET_LIB} 
		INTERFACE ${KTL_INCLUDE_DIR}
)
target_link_libraries(
	${TARGET_LIB} 
		INTERFACE basic_runtime_interface
)