cmake_minimum_required (VERSION 3.0)
project ("Kernel Template Library")

cmake_policy(SET CMP0074 NEW) # Use ${WDK_ROOT} if it's already set
 
set(KTL_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

list(APPEND CMAKE_MODULE_PATH ${KTL_DIR}/FindWDK/cmake)
find_package(WDK REQUIRED)

add_subdirectory(runtime)
add_subdirectory(include)
add_subdirectory(modules)
add_subdirectory(src)
