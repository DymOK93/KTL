cmake_minimum_required (VERSION 3.0)
project ("LockFree Library")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_LOCKFREE_DIR "${KTL_MODULES_DIR}/lockfree")

set(TARGET_LIB lockfree)

set(
	KTL_LOCKFREE_HEADER_FILES
		"node_allocator.hpp"
		"queue.hpp"
		"tagged_pointer.hpp"
)

add_library(${TARGET_LIB} INTERFACE)
target_include_directories(
	${TARGET_LIB} 
		INTERFACE ${KTL_LOCKFREE_DIR}
)
target_link_libraries(
	${TARGET_LIB} 
		INTERFACE basic_runtime_interface
		INTERFACE cpp_runtime_interface
)
