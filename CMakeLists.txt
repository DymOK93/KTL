cmake_minimum_required (VERSION 3.0)
project ("Kernel Template Library")

cmake_policy(SET CMP0074 NEW) # Use ${WDK_ROOT} if it's already set
 
set(KTL_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sys")

list(APPEND CMAKE_MODULE_PATH "${KTL_DIR}/FindWDK/cmake")
find_package(WDK REQUIRED)

if (NOT DEFINED TARGET_WINVER)
	set(TARGET_WINVER 0x602)
endif()

set(
	FMT_COMPILE_DEFINITIONS
		FMT_HEADER_ONLY
		FMT_EXCEPTIONS
		FMT_STATIC_THOUSANDS_SEPARATOR
		FMT_USE_NONTYPE_TEMPLATE_PARAMETERS=0	# For old MSVC versions (<= 16.28)
)  
set(
	BASIC_COMPILE_OPTIONS
		/MP		# Multiprocessor compilation
		/W4		# Warnings level
		/WX		# Treat warnings as errors
)
set(
	RELEASE_COMPILE_OPTIONS
		/O2		# Max optimization level
		/Ob3	# Aggressive inlining
		/Oi     # Enable intrinsics
		/Ot     # Prefer fast code
		/Oy		# Omit frame pointers
)
set(
	LTO_COMPILE_OPTIONS
		/Gy		# Using COMDAT folding for functions
		/Gw		# Using COMDAT folding for variables
		/GL		# Enable link time code generation
)
set(
	RELEASE_LINK_OPTIONS
		/OPT:REF							# Eliminate unused code
		/INCLUDE:verify_seh					# Force include symbols erroneously eliminated due to a bug in LTO (MSVC <= 16.29)
		/INCLUDE:verify_seh_in_cxx_handler
)
set(
	LTO_LINK_OPTIONS
		/OPT:ICF			# Enable COMDAT folding
		/LTCG:INCREMENTAL	# Force fast link timecode generation
)

add_subdirectory(include)
add_subdirectory(modules)
add_subdirectory(runtime)
add_subdirectory(src)
add_subdirectory(tests)
