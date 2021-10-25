# KTL
Kernel Template Library is open-source library providing CRT environment, STL-style containers and RAII tools for Windows Kernel programming.

* [Features](#features)
  * [C Runtime environment](#build-requirements)
  * [C++ Standard Library implementation](#C++-standard-library-implementation)
  * [CMake](#cmake)
* [Installation & Usage](#installation-&-Usage)
  * [CMakeLists.txt to build with a pre-built KTL](#cmakelists.txt-to-build-with-a-pre-built-ktl)
  * [Build requirements](#build-requirements)
* [Examples](#samples)
* [Roadmap for the near future](#roadmap-for-the-near-future)
* [License](#license)


# Features

* C Runtime environment
    * **Exception handling mechanism** (now for x64 only)
    * Buffer security checks
    * C++ Standard compatible termination if execution is run out of control 
    * Construction and destruction of the non-trivial static objects
    * Memory allocation using new and delete
    * Filesystem Mini-Filter support routines


* C++ Standard Library implementation
    * `<atomic>` (now for x86 and x64 only)
    * Optimized, C++ Standard compatible `<algorithm>` library
    * `<allocator>` with standard allocators for different pool types
    * Boost-based implementation of the `compressed_pair`
    * Exceptions objects hierarchy (`std::exception` analog optimized for use in the kernel)
    * Iterators
    * MSVC-intrinsic-based coroutines
    * Mutexes, events and condition variables based on kernel synchronization primitives with RAII wrappers
    * Smart pointers (`unique_ptr`, `shared_ptr` and `weak_ptr`, `intrusive_ptr`)
    * `<type_traits>`
    * `<thread>` for managing driver-dedicated threads
    * `<tuple>`
    * `<optional>` with constexpr support
    * `unordered_node_map`, `unordered_node_set`, `unordered_flat_map` and `unordered_flat_set` using [robin-hood-hashing](https://github.com/martinus/robin-hood-hashing)
    * `<vector>`
    * Lock-free queue, `node_allocator` and some auxiliary algorithms 
    * [fmt](https://github.com/fmtlib/fmt/) as a string formatting library 
    * Designed in C++17, feel free to build with C++20


* CMake
    * Building of kernel drivers
    * Generating a test-signing certificate
    * Driver signing 

_Complete documentation in progress_.

# Installation & Usage
You can use KTL directly as driver CMake project subdirectory or link with pre-built KTL binaries applying `find_package()`.

It includes **3 static libraries**: 
* `basic_runtime.lib` (CRT)
* `cpp_runtime.lib` (C++ tools)
* `minifilter_runtime.lib` (optional Filesystem Mini-Filter support)

### CMakeLists.txt to build with a pre-built KTL:
```cmake
    project(MyDriver CXX)

    find_package(WDK REQUIRED)  # Windows Driver Kit
    find_package(KTL REQUIRED)

    wdk_add_driver(
      ${DRIVER_NAME} minifilter.cpp
        CUSTOM_ENTRY_POINT KtlDriverEntry   # Required for C and C++ Runtime initialization and destruction
		    EXTENDED_CPP_FEATURES               # Exceptions handling enabled
    )
    target_include_directories(
	    ${DRIVER_NAME} PUBLIC
		    ${KTL_INCLUDE_DIR}
		    ${KTL_MODULES_DIR}                  # Lock-Free tools
		    ${KTL_RUNTIME_INCLUDE_DIR}
		    ${CMAKE_INCLUDE_CURRENT_DIR}
    )
    target_link_libraries(
      ${DRIVER_NAME} PRIVATE
        ktl::basic_runtime
        ktl::minifilter_runtime
        ktl::cpp_runtime
    )
    wdk_sign_driver(        # Driver signing (in this example - using given certificate)
	    ${DRIVER_NAME}
		    ${YOUR_CERTIFICATE_NAME}
		    CERTIFICATE_PATH ${YOUR_CERTIFICATE_PATH}
		    TIMESTAMP_SERVER ${YOUR_TIMESTAMP_SERVER} # Default is timestamp.verisign.com
    )
```

### Build requirements:
* WDK10
* Visual Studio 2019 (not tested on older versions)
* CMake 3.0 and higher
* [FindWDK](https://github.com/DymOK93/FindWDK/tree/develop) (used as a Git submodule)

# Examples
* [CoroDriverSample](https://github.com/DymOK93/CoroDriverSample) - a simple driver demonstrating the use of C++20 coroutines in kernel mode 

# Roadmap for the near future 
* Linked lists and Red-Black-Tree containers
* Intrusive containers
* Coroutine-compatible async primitives
* **Exception handling on the x86 platforms**

# License
KTL distributed under MIT [license](https://github.com/DymOK93/KTL/blob/master/LICENSE.md).
