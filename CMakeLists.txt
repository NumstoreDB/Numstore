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
option(OPTIMIZE_CODE   "Build in optimized mode"               ${_ns_debug})
option(ENABLE_COVERAGE "Enable coverage (GCC/Clang only)"      OFF)
option(BUILD_TOOLS     "Build numstore CLI tools"              ON)
option(BUILD_SAMPLES   "Build sample programs"                 ON)

unset(_ns_debug)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INSTALL_DO_STRIP TRUE)
    add_compile_definitions(NDEBUG)
endif()

if(ENABLE_TESTS)
  add_compile_definitions(TESTING)
endif()

if(NOT ENABLE_LOGGING)
  add_compile_definitions(NLOG)
endif()

########################################################################
# Compiler Flags
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
# CMake Dependencies
########################################################################
add_subdirectory(src)
add_subdirectory(src/apps)

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

install(FILES "${PROJECT_SOURCE_DIR}/src/numstore.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES "${PROJECT_SOURCE_DIR}/src/smartfiles.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES "${PROJECT_SOURCE_DIR}/src/compile_config.h"
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
