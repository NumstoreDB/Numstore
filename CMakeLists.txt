################### Meta Information
cmake_minimum_required(VERSION 3.18)
enable_testing()

## Read the version from version.txt
file(STRINGS "${CMAKE_SOURCE_DIR}/version.txt" NUMSTORE_VERSION LIMIT_COUNT 1)

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
configure_file(
    "${CMAKE_SOURCE_DIR}/templates/compile_config.h.in"
    "${CMAKE_SOURCE_DIR}/include/compile_config.h"
    @ONLY
)

################### Numstore Library
file(
  GLOB_RECURSE NUMSTORE_SOURCES  
  CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c"
)
file(
  GLOB_RECURSE POSIX_SOURCES 
  CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/posix/*.c"
)
file(
  GLOB_RECURSE WINDOWS_SOURCES 
  CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/windows/*.c"
)

list(REMOVE_ITEM NUMSTORE_SOURCES ${POSIX_SOURCES})
list(REMOVE_ITEM NUMSTORE_SOURCES ${WINDOWS_SOURCES})

if(WIN32)
    list(APPEND NUMSTORE_SOURCES ${WINDOWS_SOURCES})
    set(PLATFORM_DEPENDENCIES ws2_32)
else()
    list(APPEND NUMSTORE_SOURCES ${POSIX_SOURCES})
    set(PLATFORM_DEPENDENCIES pthread)
endif()

add_library(numstore STATIC ${NUMSTORE_SOURCES})
target_include_directories(numstore
  PUBLIC  "${CMAKE_SOURCE_DIR}/include"
  PRIVATE "${CMAKE_SOURCE_DIR}/src"
)
target_link_libraries(numstore PUBLIC ${PLATFORM_DEPENDENCIES})

################### Sub Directories
add_subdirectory(apps)

if(BUILD_PYTHON_BINDINGS)
  add_subdirectory(bindings/python/pynumstore)
endif()

