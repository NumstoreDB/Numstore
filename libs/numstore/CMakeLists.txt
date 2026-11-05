#####################################################################
### The numstore library
### One target: core/ and nscore/ keep their own CMakeLists for readability
### but only append to LIBNUMSTORE_SRCS.

set(LIBNUMSTORE_SRCS "")
add_subdirectory(src/core)
add_subdirectory(src/nscore)

# src/numstore.c is the public C API (include/numstore.h). pynumstore talks
# to nscore/nsdb directly, so a wheel build leaves it out entirely.
if(NOT SKBUILD)
    list(APPEND LIBNUMSTORE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/numstore.c)
endif()

# The root needs this to depend the generated unit tests on the sources.
set(LIBNUMSTORE_SRCS ${LIBNUMSTORE_SRCS} PARENT_SCOPE)

add_library(numstore STATIC ${LIBNUMSTORE_SRCS})
add_library(numstore::numstore ALIAS numstore)

# include/ is flat, so the public header stays #include "numstore.h". src/ is
# there too because the internal headers include each other as "core/..." and
# "nscore/...".
target_include_directories(
    numstore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(numstore PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    target_compile_options(numstore PUBLIC /experimental:c11atomics)
endif()

# Prefer using -pthread instead of -lpthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(numstore PUBLIC Threads::Threads)

# long double / complex helpers are in libm on glibc.
if(UNIX AND NOT APPLE)
    target_link_libraries(numstore PUBLIC m)
endif()

# Install
install(
    TARGETS numstore
    EXPORT numstoreTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# The installed package carries the version from version.txt, so a consumer can
# pin it with find_package(numstore 1.5).
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/numstoreConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/numstoreConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/numstore
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/numstoreConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
)

install(
    EXPORT numstoreTargets
    FILE numstoreTargets.cmake
    NAMESPACE numstore::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/numstore
)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/numstoreConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/numstoreConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/numstore
)
