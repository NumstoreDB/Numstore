cmake_minimum_required(VERSION 3.20)

# Project version parsing
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" NUMSTORE_VERSION)
string(STRIP "${NUMSTORE_VERSION}" NUMSTORE_VERSION)
string(REGEX REPLACE "^v" "" NUMSTORE_VERSION "${NUMSTORE_VERSION}")
string(
    REGEX MATCH "^[0-9]+(\\.[0-9]+)*"
    NUMSTORE_CMAKE_VERSION
    "${NUMSTORE_VERSION}"
)
project(numstore VERSION ${NUMSTORE_CMAKE_VERSION} LANGUAGES C)

include(GNUInstallDirs)
include(cmake/FeatureFlags.cmake)

# Rust always links the release CRT (/defaultlib:msvcrt), even in debug builds.
# A Debug C library built with /MDd references the debug CRT (_CrtDbgReport,
# _invalid_parameter) and fails to link into the Rust test binary, so use the
# release DLL CRT in every configuration.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_definitions($<$<CONFIG:Release>:NLOG>)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

#####################################################################
### The numstore libraries
add_subdirectory(libs)

#####################################################################
### pynumstore
### scikit-build-core defines SKBUILD, and a wheel build wants nothing else.
if(SKBUILD)
    find_package(
        Python
        3.9
        REQUIRED
        COMPONENTS Interpreter Development.Module NumPy
    )

    python_add_library(
        _pynumstore
        MODULE
        WITH_SOABI
        libs/pynumstore/ns_pynumstore.c
    )
    # pynumstore sits on the nsdb layer, not on the public ns_* api, so it
    # needs ns_nsdb (and what that links to) and nothing of ns_numstore
    target_link_libraries(_pynumstore PRIVATE ns_nsdb Python::NumPy)
    set_target_properties(_pynumstore PROPERTIES C_VISIBILITY_PRESET hidden)

    install(TARGETS _pynumstore LIBRARY DESTINATION pynumstore COMPONENT python)

    return()
endif()

#####################################################################
### Rust setup
include(cmake/Rust.cmake)

corrosion_set_env_vars(
    numstore_rs
    NUMSTORE_LIB_DIR=$<TARGET_FILE_DIR:ns_numstore>
)
add_dependencies(_cargo-build_numstore_rs ns_numstore)

#####################################################################
### apps
add_subdirectory(apps)

#####################################################################
### samples
file(GLOB NUMSTORE_SAMPLES CONFIGURE_DEPENDS samples/*.c)

foreach(sample IN LISTS NUMSTORE_SAMPLES)
    get_filename_component(name ${sample} NAME_WE)
    add_executable(${name} ${sample})
    target_link_libraries(${name} PRIVATE ns_numstore)
endforeach()

#####################################################################
### docs
### Off by default: the HTML site needs pandoc, which a code build does not.
option(NUMSTORE_DOCS "Render docs/ to HTML with pandoc" OFF)

if(NUMSTORE_DOCS)
    add_subdirectory(docs)
endif()
