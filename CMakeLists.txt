cmake_minimum_required (VERSION 3.0)
project ("Kernel Template Library")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(WDK REQUIRED)

set(KTL_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(KTL_RUNTIME_DIR "${KTL_DIR}/runtime")
set(KTL_INCLUDE_DIR "${KTL_DIR}/include")
set(KTL_SOURCE_DIR "${KTL_DIR}/src")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

add_subdirectory(runtime)
add_subdirectory(include)
add_subdirectory(src)
