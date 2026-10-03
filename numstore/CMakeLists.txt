cmake_minimum_required(VERSION 3.18)
project(numstore LANGUAGES C)

include(GNUInstallDirs)

# ns_platform.h uses <stdatomic.h> and <complex.h>, so C11 is the minimum.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# for MSVC: as of 19.51 its <stdatomic.h> is still gated behind /experimental:c11atomics
if(MSVC)
    add_compile_options(/experimental:c11atomics)
endif()

# NLOG compiles out leveled logging. Test CASE/PASSED/FAILURE lines survive.
option(NS_NLOG "Compile out logging" OFF)
if(NS_NLOG)
    add_compile_definitions(NLOG)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

#####################################################################
### The main numstore library
add_library(numstore.library STATIC)
add_library(numstore::numstore ALIAS numstore.library)
target_include_directories(
    numstore.library
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_target_properties(
    numstore.library
    PROPERTIES
        OUTPUT_NAME numstore
        EXPORT_NAME numstore
        POSITION_INDEPENDENT_CODE ON
)

# Prefer using -pthread instead of -lpthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(numstore.library PUBLIC Threads::Threads)

# long double / complex helpers land in libm on glibc.
if(UNIX AND NOT APPLE)
    target_link_libraries(numstore.library PUBLIC m)
endif()

install(
    TARGETS numstore.library
    EXPORT numstoreTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#####################################################################
### Numstore Internal
### Internal view of numstore: also exposes the private src/ headers.
### Only used by in-tree tools; never installed or exported.
add_library(numstore.library.private INTERFACE)
target_link_libraries(numstore.library.private INTERFACE numstore.library)
target_include_directories(
    numstore.library.private
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

#####################################################################
### Traverse the source tree
add_subdirectory(src)
add_subdirectory(apps)
