cmake_minimum_required(VERSION 3.20)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" NUMSTORE_VERSION)
string(STRIP "${NUMSTORE_VERSION}" NUMSTORE_VERSION)
string(REGEX REPLACE "^v" "" NUMSTORE_VERSION "${NUMSTORE_VERSION}")

# project() only takes digits, so a pre-release suffix (1.5.0rc1) is trimmed
# here. The wheel still gets the full string from version.txt.
string(
    REGEX MATCH "^[0-9]+(\\.[0-9]+)*"
    NUMSTORE_CMAKE_VERSION
    "${NUMSTORE_VERSION}"
)

project(numstore VERSION ${NUMSTORE_CMAKE_VERSION} LANGUAGES C)

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_definitions($<$<CONFIG:Release>:NLOG>)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

#####################################################################
### The numstore library
add_subdirectory(libs/numstore)

#####################################################################
### pynumstore
### scikit-build-core defines SKBUILD, and a wheel build wants nothing else.
if(SKBUILD)
    add_subdirectory(libs/pynumstore)
    return()
endif()

#####################################################################
### Rust
### Always built. A wheel build returns above this, so cibuildwheel still
### needs no cargo.
include(cmake/Rust.cmake)

#####################################################################
### apps
add_subdirectory(apps)

#####################################################################
### samples
file(GLOB NUMSTORE_SAMPLES CONFIGURE_DEPENDS samples/*.c)

foreach(sample IN LISTS NUMSTORE_SAMPLES)
    get_filename_component(name ${sample} NAME_WE)
    add_executable(${name} ${sample})
    target_link_libraries(${name} PRIVATE numstore)
endforeach()

#####################################################################
### unit_tests
### gen_tests.py scrapes TEST(...) out of the library sources and writes the
### main(). The source root is an argument so the script holds no paths.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    set(_unit_tests_c ${CMAKE_CURRENT_BINARY_DIR}/unit_tests.c)

    # Which os backend's tests to scrape. WIN32 is the target, not the host, so
    # this stays right under a mingw cross build.
    if(WIN32)
        set(_unit_tests_platform windows)
    else()
        set(_unit_tests_platform posix)
    endif()

    add_custom_command(
        OUTPUT ${_unit_tests_c}
        COMMAND
            ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/apps/gen_tests.py
            ${CMAKE_CURRENT_SOURCE_DIR}/apps/unit_tests.c.in ${_unit_tests_c}
            ${_unit_tests_platform}
            ${CMAKE_CURRENT_SOURCE_DIR}/libs/numstore/src
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/apps/gen_tests.py
            ${CMAKE_CURRENT_SOURCE_DIR}/apps/unit_tests.c.in
            ${LIBNUMSTORE_SRCS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating unit_tests.c"
        VERBATIM
    )

    add_executable(unit_tests ${_unit_tests_c})
    target_link_libraries(unit_tests PRIVATE numstore)
endif()

#####################################################################
### docs
### Off by default: the HTML site needs pandoc, which a code build does not.
option(NUMSTORE_DOCS "Render docs/ to HTML with pandoc" OFF)

if(NUMSTORE_DOCS)
    add_subdirectory(docs)
endif()
