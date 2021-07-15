cmake_minimum_required (VERSION 3.0)
project ("Kernel Template Library")

cmake_policy(SET CMP0074 NEW) # Use ${WDK_ROOT} if it's already set
 
set(KTL_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

list(APPEND CMAKE_MODULE_PATH ${KTL_DIR}/FindWDK/cmake)
find_package(WDK REQUIRED)

set(
	BASIC_COMPILE_OPTIONS
		/MP	# Multiprocessor compilation
		/W4 # Warnings level
		/WX # Treat warnings as errors
)
set(
	RELEASE_COMPILE_OPTIONS
		/O2		# Max optimization level
		/Ob3	# Aggressive inlining
		/Oi     # Enable intrinsics
		/Ot     # Prefer fast code
)
set(
	RELEASE_LINK_OPTIONS
		/OPT:REF # Eliminate unused code
)

add_subdirectory(runtime)
add_subdirectory(include)
add_subdirectory(modules)
add_subdirectory(src)

