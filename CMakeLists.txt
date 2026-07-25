########################################################################
# numstore
#     All the meta information and dependency's 
#     and high level things like artifact locations
########################################################################
cmake_minimum_required(VERSION 3.18)

file(STRINGS version.txt NUMSTORE_VERSION LIMIT_COUNT 1)
project(numstore
    VERSION      "${NUMSTORE_VERSION}"
    DESCRIPTION  "Custom database engine for arrays"
    HOMEPAGE_URL "https://github.com/NumstoreDB/Numstore"
    LANGUAGES    C
)

# Default Debug build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release Package)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

include(GNUInstallDirs)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

########################################################################
# Options
#     These are all the configurable options you can pass 
#     on the command line - Numstore's philosophy is keep 
#     compile time configuration lightweight - you should 
#     usually not be touching these things
########################################################################
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(_ns_debug OFF)
else()
    set(_ns_debug ON)
endif()

option(ENABLE_PORTABLE "Portable build"                        OFF)
option(ENABLE_TESTS    "Compile self-tests into the library"   ${_ns_debug})
option(ENABLE_LOGGING  "Compile in runtime logging"            ${_ns_debug})
option(ENABLE_ASAN     "Enable AddressSanitizer"               ${_ns_debug})
option(OPTIMIZE_CODE   "Build in optimized mode"               ${_ns_debug})
option(ENABLE_COVERAGE "Enable coverage (GCC/Clang only)"      OFF)
option(BUILD_TOOLS     "Build numstore CLI tools"              ON)
option(BUILD_SAMPLES   "Build sample programs"                 ON)
option(BUILD_DOCS      "Build documentation"                   ON)

unset(_ns_debug)

########################################################################
# Compiler Flags
########################################################################

include(compiler_flags)

########################################################################
# CMake Dependencies
########################################################################
add_subdirectory(src)
if(BUILD_DOCS) 
  add_subdirectory(docs)
endif()
