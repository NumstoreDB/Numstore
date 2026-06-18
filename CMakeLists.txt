########################################################################
# numstore
#     All the meta information and dependency's 
#     and high level things like artifact locations
########################################################################
cmake_minimum_required(VERSION 3.18)
enable_testing()

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
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

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
option(ENABLE_COVERAGE "Enable coverage (GCC/Clang only)"      OFF)
option(BUILD_TOOLS     "Build numstore CLI tools"              ON)
option(BUILD_SAMPLES   "Build sample programs"                 ON)

unset(_ns_debug)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INSTALL_DO_STRIP TRUE)
    add_compile_definitions(NDEBUG)
endif()

if(NOT ENABLE_TESTS)
  add_compile_definitions(NTEST)
endif()

if(NOT ENABLE_LOGGING)
  add_compile_definitions(NLOG)
endif()


########################################################################
# Compile-time config (consumed by src/templates/compile_config.h.in)
#     These are all the configurable compile time constants 
#     in numstore - things like the number of bits per data element 
#     PAGE_SIZE, memory pool size etc.
########################################################################
include(ProcessorCount)
ProcessorCount(NS_CPU_COUNT)
if(NS_CPU_COUNT EQUAL 0)
    set(NS_CPU_COUNT 1)
endif()

set(NS_VERSION_MAJOR  ${PROJECT_VERSION_MAJOR})
set(NS_VERSION_MINOR  ${PROJECT_VERSION_MINOR})
set(NS_VERSION_PATCH  ${PROJECT_VERSION_PATCH})
set(NS_VERSION_STRING "${PROJECT_VERSION}")

set(NS_PAGE_SIZE       4096    CACHE STRING "Page size")
set(NS_MAX_OPEN_FILES  20      CACHE STRING "Maximum number of open files")
set(NS_MEMORY_PAGE_LEN 4096    CACHE STRING "Number of buffer pool slots")
set(NS_WAL_BUFFER_CAP  1048576 CACHE STRING "WAL buffer capacity in bytes")

set(TSIZE_BITS 32 CACHE STRING "Index into a type")
set(PSIZE_BITS 32 CACHE STRING "Index into a page")
set(BSIZE_BITS 64 CACHE STRING "Index into a byte stream or array")
set(PGNO_BITS  64 CACHE STRING "Page number")
set(TXID_BITS  64 CACHE STRING "Transaction id")
set(LSN_BITS   64 CACHE STRING "WAL log sequence number")
set(PGH_BITS   8  CACHE STRING "Page header size")
set(WLH_BITS   8  CACHE STRING "WAL header size")

configure_file(
    "${PROJECT_SOURCE_DIR}/src/templates/compile_config.h.in"
    "${PROJECT_SOURCE_DIR}/src/compile_config.h"
    @ONLY
)

########################################################################
# Windows / MSVC
########################################################################
if(MSVC)
    add_compile_options(
      /W3
      /wd4100 /wd4101 /wd4244 /wd4267
      /experimental:c11atomics
    )

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
      add_compile_options(/Od /Zi)
    endif()

    if(ENABLE_ASAN)
        add_compile_options(/fsanitize=address /Zi)
        add_link_options(/INCREMENTAL:NO)
    endif()

    if(NOT ENABLE_PORTABLE)
        add_compile_options(/arch:AVX2)
    endif()

    if(ENABLE_COVERAGE)
        message(FATAL_ERROR "Coverage is only supported on GCC/Clang.")
    endif()

########################################################################
# GCC / Clang
########################################################################
else()
    add_compile_options(
        -Wall -Wextra -Werror
        -Wshadow -Wsign-compare
        -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
        -pedantic-errors
        -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable
    )

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
      add_compile_options(-g -O0)
    else()
      add_compile_options(-O3)
    endif()

    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        add_compile_options(
            -Wno-gnu-zero-variadic-macro-arguments
            -Wno-static-in-inline
        )
    endif()

    if(ENABLE_ASAN)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=address)
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            add_compile_options(-fsanitize=leak)
            add_link_options(-fsanitize=leak)
        endif()
    endif()

    if(ENABLE_COVERAGE)
        add_compile_options(--coverage -fprofile-update=atomic)
        add_link_options(--coverage)
    endif()

    if(ENABLE_PORTABLE)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
            add_compile_options(-march=x86-64 -mtune=generic)
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
            add_compile_options(-march=armv8-a -mtune=generic)
        endif()
    else()
        add_compile_options(-march=native -mtune=native)
    endif()
endif()

########################################################################
# Platform link deps
########################################################################
if(WIN32)
    set(PLATFORM_DEPENDENCIES ws2_32)
else()
    find_package(Threads REQUIRED)
    set(PLATFORM_DEPENDENCIES Threads::Threads)
endif()

########################################################################
# Numstore Library
########################################################################
file(GLOB NUMSTORE_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.c")
file(GLOB NUMSTORE_TESTING_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/testing/*.c")

if(ENABLE_TESTS)
    add_library(numstore ${NUMSTORE_SOURCES} ${NUMSTORE_TESTING_SOURCES})
else()
    add_library(numstore ${NUMSTORE_SOURCES})
endif()

add_library(numstore::numstore ALIAS numstore)

set_target_properties(numstore PROPERTIES 
    VERSION   ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(numstore PRIVATE "${PROJECT_SOURCE_DIR}/src")

target_link_libraries(numstore PUBLIC ${PLATFORM_DEPENDENCIES})

########################################################################
# Install rules
########################################################################
install(TARGETS numstore
    EXPORT   numstoreTargets
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES "${PROJECT_BINARY_DIR}/src/numstore.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES "${PROJECT_BINARY_DIR}/src/smartfiles.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES "${PROJECT_BINARY_DIR}/src/compile_config.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# CMake package config — enables find_package(numstore)
install(EXPORT numstoreTargets
    FILE      numstoreTargets.cmake
    NAMESPACE numstore::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/numstore
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/numstoreConfigVersion.cmake"
    VERSION       ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/src/templates/numstoreConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/numstoreConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/numstore
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/numstoreConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/numstoreConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/numstore
)

if(WIN32)
    set(NUMSTORE_PC_PRIVATE_LIBS "-lws2_32")
else()
    set(NUMSTORE_PC_PRIVATE_LIBS "-lpthread")
endif()

# pkg-config — enables `pkg-config --libs numstore`
configure_file(
    "${PROJECT_SOURCE_DIR}/src/templates/numstore.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/numstore.pc"
    @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/numstore.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

############################## Add Numstore Executable
# Creates an executable target, sets up standard include directories,
# links it against the internal numstore library, and installs it.
# This targets headers against all private and public includes.
#
# NAME
#   The target name for the generated executable
#
# FILE
#   The path to the primary source file (e.g., main.c)
#
# Example:
#   ns_add_executable(my_app src/main.c)
#
function(ns_add_executable NAME FILE)
  add_executable(${NAME} ${FILE})
  target_include_directories(${NAME}
    PRIVATE "${PROJECT_SOURCE_DIR}/src"
  )
  target_link_libraries(${NAME} PRIVATE numstore::numstore)
  install(TARGETS ${NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction()

############################## Add Numstore Sample
# Creates a single sample executable target, includes the project
# header directory, and links it against the core library.
# Only links public headers.
#
# NAME
#   The target name for the generated executable.
#
# FILE
#   The path to the source file (e.g., sample.c).
#
function(ns_add_sample NAME FILE)
  add_executable(${NAME} ${FILE})
  target_include_directories(${NAME} PRIVATE "${PROJECT_SOURCE_DIR}/src")
  target_link_libraries(${NAME} PRIVATE numstore::numstore)
  install(TARGETS ${NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction()

########################################################################
# Testing Code
########################################################################
if(ENABLE_TESTS)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)

  # See apps/scripts/gen_tests.py — scans src/ for TEST(...) markers and
  # generates a runner from the template.
  set(GENERATED_UNIT_TESTS "${PROJECT_SOURCE_DIR}/apps/testing/unit_tests.c")
  execute_process(
    COMMAND ${Python3_EXECUTABLE}
            "${PROJECT_SOURCE_DIR}/apps/scripts/gen_tests.py"
            "${PROJECT_SOURCE_DIR}/src/templates/unit_tests.c.in"
            "${GENERATED_UNIT_TESTS}"
            "${PROJECT_SOURCE_DIR}/src"
    RESULT_VARIABLE gen_result
    OUTPUT_VARIABLE gen_stdout
    ERROR_VARIABLE  gen_stderr
  )
  if(NOT gen_result EQUAL 0)
    message(FATAL_ERROR
      "gen_tests.py failed (${gen_result}):\n${gen_stdout}\n${gen_stderr}")
  endif()

  # ----- Build the test executables -----
  ns_add_executable(cgd_swarm_test  apps/testing/cgd_swarm_test.c)
  ns_add_executable(irwr_swarm_test apps/testing/irwr_swarm_test.c)
  ns_add_executable(unit_tests      "${GENERATED_UNIT_TESTS}")

  # ----- Register with CTest -----
  # Both swarm tests take a single integer argument (seconds).
  # Override at configure time with -DNS_SWARM_TIMEOUT=<n>.
  set(NS_SWARM_TIMEOUT 20 CACHE STRING "Seconds for swarm tests")

  add_test(NAME unit_tests      COMMAND unit_tests)
  add_test(NAME cgd_swarm_test  COMMAND cgd_swarm_test  ${NS_SWARM_TIMEOUT})
  add_test(NAME irwr_swarm_test COMMAND irwr_swarm_test ${NS_SWARM_TIMEOUT})
endif()

########################################################################
# Tools — simple UNIX-like debug tools 
########################################################################
if(BUILD_TOOLS)
  ns_add_executable(dlread          apps/tools/dlread.c)
  ns_add_executable(nspprint        apps/tools/nspprint.c)
  ns_add_executable(simple_nspprint apps/tools/simple_nspprint.c)
  ns_add_executable(walfprint       apps/tools/walfprint.c)
endif()

########################################################################
# Samples — pedagogical programs for numstore newcomers
########################################################################
if(BUILD_SAMPLES)
  ns_add_sample(ns_sample1_basic_crud          apps/samples/numstore/sample1_basic_crud.c)
  ns_add_sample(smfile_sample1_basic_crud      apps/samples/smartfiles/sample1_basic_crud.c)
  ns_add_sample(smfile_sample2_transactions    apps/samples/smartfiles/sample2_transactions.c)
  ns_add_sample(smfile_sample3_stride          apps/samples/smartfiles/sample3_stride.c)
  ns_add_sample(smfile_sample4_variable_names  apps/samples/smartfiles/sample4_variable_names.c)
  ns_add_sample(smfile_sample5_rollback_commit apps/samples/smartfiles/sample5_rollback_commit.c)
endif()

########################################################################
# CPack — .tar.gz / .zip / .deb / .rpm
########################################################################
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME                "numstore")
set(CPACK_PACKAGE_VENDOR              "NumStore Project")
set(CPACK_PACKAGE_CONTACT             "lincketheo@gmail.com")
set(CPACK_PACKAGE_HOMEPAGE_URL        "${PROJECT_HOMEPAGE_URL}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_RESOURCE_FILE_LICENSE       "${PROJECT_SOURCE_DIR}/LICENSE.md")
set(CPACK_RESOURCE_FILE_README        "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_STRIP_FILES                 TRUE)
set(CPACK_VERBATIM_VARIABLES          YES)

# Debian
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Theo Lincke <lincketheo@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION    "database")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS  ON)
set(CPACK_DEBIAN_FILE_NAME          "DEB-DEFAULT")

# RPM
set(CPACK_RPM_PACKAGE_LICENSE  "Apache-2.0")
set(CPACK_RPM_PACKAGE_GROUP    "Applications/Databases")
set(CPACK_RPM_PACKAGE_AUTOREQ  YES)
set(CPACK_RPM_FILE_NAME        "RPM-DEFAULT")

# Source archives
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_IGNORE_FILES
    "/\\.git/" "/build/" "/target/" "\\.DS_Store"
)

if(NOT CPACK_GENERATOR)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(CPACK_GENERATOR "TGZ;DEB;RPM")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(CPACK_GENERATOR "ZIP")
    else()
        set(CPACK_GENERATOR "TGZ")
    endif()
endif()

include(CPack)
