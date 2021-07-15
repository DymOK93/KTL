# KTL
Kernel Template Library is open-source library providing CRT environment, STL-style containers and RAII tools for Windows Kernel programming

# Features
* C Runtime environment
    * Exception handling mechanism (now for x64 only)
    * Buffer security checks
    * C++ Standard compatible termination if execution is run out of control 
    * Construction and destruction of the non-trivial static objects
    * Memory allocation using new and delete
    * Filesystem Mini-Filter support routines
* C++ Tools
    * Atomics (now for x86 and x64 only)
    * Optimized, C++ Standard compatible algorithms for strings and iterator ranges
    * Allocators support with standard allocators for different pool types
    * Boost-based implementation of the compressed_pair
    * Exceptions objects hierarchy (std::exception analogue optimized for use in the kernel)
    * Iterators
    * MSVC-intrinsic-based coroutines
    * Mutexes, events and condition variables based on kernel synchronization primitives with RAII wrappers
    * Unique and shared pointers
    * Type traits
    * Tuple
    * Optional with constexpr support
    * Unordered map, set, flat map and flat set (based on [robin-hood-hashing)](https://github.com/martinus/robin-hood-hashing))
    * Vector
    * Lock-free queue, node allocator and some auxiliary algorithms 
    * Building with C++20 
* CMake
    * Building of kernel drivers
    * Generating a test-signing certificate
    * Driver signing 


_Complete documentation in progress_

# Examples
_Examples in progress_

# Installation & Usage
You can use KTL directly as driver CMake project subdirectory or link with prebuilt KTL binaries applying `find_package()`
It includes 3 static libraries: 
* `basic_runtime.lib` (CRT)
* `cpp_runtime.lib` (C++ tools)
* `minifilter_runtime.lib` (optional Filesystem Mini-Filter)

CMakeLists.txt example:
```
    project(MyDriver CXX)

    find_package(WDK REQUIRED)  # Windows Driver Kit
    find_package(KTL REQUIRED)

    wdk_add_driver(
        ${DRIVER_NAME} minifilter.cpp
            CUSTOM_ENTRY_POINT KtlDriverEntry   # Required for C and C++ Runtime initialization and destruction
		    EXTENDED_CPP_FEATURES               # Exceptions handling enabled
    )
    target_include_directories(
	    ${TARGET_DRIVER} PUBLIC
		    ${KTL_INCLUDE_DIR}
		    ${KTL_MODULES_DIR}                  # Lock-Free
		    ${KTL_RUNTIME_INCLUDE_DIR}
		    ${CMAKE_INCLUDE_CURRENT_DIR}
    )
    target_link_libraries(
        ${PROJECT_NAME} PRIVATE
            ktl::basic_runtime
            ktl::minifilter_runtime
            ktl::cpp_runtime
    )
    wdk_sign_driver(        # Driver signing (in this example - using given certificate)
	    ${TARGET_DRIVER}
		    ${YOUR_CERTIFICATE_NAME}
		    CERTIFICATE_PATH ${YOUR_CERTIFICATE_PATH}
		    TIMESTAMP_SERVER ${YOUR_TIMESTAMP_SERVER} # Default is timestamp.verisign.com
    )

```

# Roadmap for the near future 
* Introduction of fmt-based string formatted library
* Linked lists and RBTree-based containers
* Intrusive smart pointers and containers
* Thread support library
* Coroutine-compatible async primitives
* Exception handling on the x86 platforms

# License
KTL distributed under MIT license
