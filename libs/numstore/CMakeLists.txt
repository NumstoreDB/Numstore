set(LIBNUMSTORE_SRCS "")

# These append to LIBNUMSTORE_SRCS
add_subdirectory(src/core)
add_subdirectory(src/nscore)

# pynumstore only uses numstore internals - not the public api
if(NOT SKBUILD)
    list(APPEND LIBNUMSTORE_SRCS src/numstore.c)
endif()

# The main library and it's aliases
add_library(numstore STATIC ${LIBNUMSTORE_SRCS})
add_library(numstore::numstore ALIAS numstore)

target_include_directories(
    numstore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(numstore PROPERTIES POSITION_INDEPENDENT_CODE ON)

# MSVC needs experimental for c11 atomics
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
