################### Meta Information
cmake_minimum_required(VERSION 3.18)

## Read the version from version.txt
file(STRINGS "${PROJECT_SOURCE_DIR}/version.txt" NUMSTORE_VERSION LIMIT_COUNT 1)

project(numstore
    VERSION     "${NUMSTORE_VERSION}"
    DESCRIPTION "Custom database engine for arrays"
    LANGUAGES   C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set the output directory for artifacts
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

################### CMake Includes
include(GNUInstallDirs)
include(cmake/Options.cmake)
include(cmake/CompilerFlags.cmake)
include(cmake/Packaging.cmake)
include(cmake/Config.cmake)

################### Auto Generate Code
# Compile Config contains compile time constants 
# like type sizes and static buffer sizes
configure_file(
    "${PROJECT_SOURCE_DIR}/templates/compile_config.h.in"
    "${PROJECT_SOURCE_DIR}/include/compile_config.h"
    @ONLY
)

################### Numstore Library
# This is the main numstore library - if you wanted 
# to compile numstore by hand - you'd follow these directions:
#
# 1. Ensure some sort of compile_config.h is generated (take a look at the auto generated code in "include")
# 2. Glob all the c files in src - some are in deeper directories to make it easier to navigate the code 
# 3. Glob your platform specific code 
# 4. Compile it all with include directory "include"
#
# Make sure you link against pthreads or ws2_32 depending on platform

## Shared Common Code
file(
  GLOB NUMSTORE_SOURCES CONFIGURE_DEPENDS 
  "${PROJECT_SOURCE_DIR}/src/*.c"
  "${PROJECT_SOURCE_DIR}/src/pager/*.c"
  "${PROJECT_SOURCE_DIR}/src/pages/*.c"
  "${PROJECT_SOURCE_DIR}/src/parsers/*.c"
  "${PROJECT_SOURCE_DIR}/src/rope/*.c"
  "${PROJECT_SOURCE_DIR}/src/var/*.c"
)

## Platform Specific Code and libraries
if(WIN32)
    file(GLOB WINDOWS_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/windows/*.c")
    list(APPEND NUMSTORE_SOURCES ${WINDOWS_SOURCES})
    set(PLATFORM_DEPENDENCIES ws2_32)
else()
    file(GLOB POSIX_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/posix/*.c")
    list(APPEND NUMSTORE_SOURCES ${POSIX_SOURCES})
    set(PLATFORM_DEPENDENCIES pthread)
endif()

## Test specific code
if(NOT NTEST)
    file(GLOB NSTEST_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/testing/*.c")
    list(APPEND NUMSTORE_SOURCES ${NSTEST_SOURCES})
endif()

## Create the Library
add_library(numstore STATIC ${NUMSTORE_SOURCES})
target_include_directories(numstore
  PUBLIC  "${PROJECT_SOURCE_DIR}/include"
  PRIVATE "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(numstore PUBLIC ${PLATFORM_DEPENDENCIES})

################### Sub Directories
# Apps are a bunch of useful developer tools and 
# samples to highlight the core usage of numstore
add_subdirectory(apps)
