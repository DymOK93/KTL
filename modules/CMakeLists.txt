cmake_minimum_required (VERSION 3.0)
project ("KTL Modules")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTL_MODULES_DIR "${KTL_DIR}/modules")

add_subdirectory(fmt)
add_subdirectory(lockfree)
