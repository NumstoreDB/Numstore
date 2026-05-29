################### Meta Information
cmake_minimum_required(VERSION 3.18)
enable_testing()

## Read the version from version.txt
file(STRINGS "${CMAKE_SOURCE_DIR}/version.txt" NS_VERSION LIMIT_COUNT 1)

project(numstore
    VERSION     "${NS_VERSION}"
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

# Windows needs this to function properly when loading dlls
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

################### CMake Includes
include(GNUInstallDirs)
include(cmake/Options.cmake)
include(cmake/CompilerFlags.cmake)
include(cmake/Packaging.cmake)
include(cmake/Config.cmake)
include(cmake/Python.cmake)

################### Core Libraries
add_subdirectory(libs/c_specx)
add_subdirectory(libs/nscore)
add_subdirectory(libs/numstore)
add_subdirectory(libs/smartfiles)
add_subdirectory(libs/nstesting)
add_subdirectory(libs/pynumstore)

################### Configuration sub projects
if(BUILD_APPS)
  add_subdirectory(apps)
endif()

if(BUILD_SAMPLES)
  add_subdirectory(samples)
endif()
