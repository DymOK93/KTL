# ----------
# 2020-2021 Dmitry Bolshakov
# ----------
# Kernel Template Library
# CRT environment, STL-style containers and RAII tools for Windows Kernel programming
# ----------

if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 3.0)
   message(FATAL_ERROR "CMake version >= 3.0 required")
endif()

set(KTL_DIR ${CMAKE_CURRENT_LIST_DIR})
set(KTL_RUNTIME_DIR "${KTL_DIR}/runtime")
set(KTL_RUNTIME_INCLUDE_DIR "${KTL_RUNTIME_DIR}/include")
set(KTL_INCLUDE_DIR "${KTL_DIR}/include")
set(KTL_SOURCE_DIR "${KTL_DIR}/src")
set(KTL_MODULES_DIR "${KTL_DIR}/modules")

file(GLOB KTL_RUNTIME_LIBRARIES "${KTL_RUNTIME_DIR}/lib/*.lib")    
file(GLOB KTL_CPP_LIBRARIES "${KTL_DIR}/lib/*.lib")  
  

foreach(LIBRARY IN LISTS KTL_RUNTIME_LIBRARIES KTL_CPP_LIBRARIES)
    get_filename_component(LIBRARY_NAME ${LIBRARY} NAME_WE)

# Protect against multiple inclusion, which would fail when already imported targets are added once more.
    if(NOT TARGET ktl::${LIBRARY_NAME})   
        add_library(ktl::${LIBRARY_NAME} INTERFACE IMPORTED)
        set_property(TARGET ktl::${LIBRARY_NAME} PROPERTY INTERFACE_LINK_LIBRARIES  ${LIBRARY})
    endif()
endforeach(LIBRARY)
unset(KTL_LIBRARIES)
