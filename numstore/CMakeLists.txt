cmake_minimum_required(VERSION 3.18)
project(numstore LANGUAGES C)

include(GNUInstallDirs)

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

# On glibc older than 2.34 the pthread entry points live in libpthread rather
# than libc, so anything linking numstore has to ask for them. Only the oldest
# targets expose this (manylinux2014 is CentOS 7 / glibc 2.17, and it failed to
# link pthread_once and pthread_join); newer glibc resolves them from libc and
# hides the omission. PUBLIC so every consumer inherits it. On Windows the
# Win32 threading backend is used and Threads::Threads resolves to nothing.
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
