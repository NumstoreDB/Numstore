set(NS_NUMSTORE_SRCS "")

# This appends to NS_NUMSTORE_SRCS
add_subdirectory(tests)

# pynumstore only uses numstore internals - not the public api
if(NOT SKBUILD)
    list(APPEND NS_NUMSTORE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/numstore.c)
endif()

# The public library: the ns_* api in include/numstore.h, and the tests that
# need the whole database
ns_add_library(ns_numstore ${NS_NUMSTORE_SRCS})
add_library(numstore::numstore ALIAS ns_numstore)
target_link_libraries(
    ns_numstore
    PUBLIC ns_common ns_types ns_disk_pager ns_page ns_algorithms ns_nsdb
)

# The one public header
target_include_directories(
    ns_numstore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
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
