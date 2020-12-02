# ----------
# 2020 Dmitry Bolshakov
# ----------
# Kernel Template Library
# CRT environment, STL-style containers and RAII tools for Windows Kernel programming
# ----------

if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 3.0)
   message(FATAL_ERROR "CMake version >= 3.0.0 required")
endif()

set(KTL_RUNTIME_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/runtime/include")
set(KTL_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

#add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/runtime")