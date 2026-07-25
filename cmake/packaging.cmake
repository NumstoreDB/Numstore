########################################################################
# Install rules
########################################################################
include(CMakePackageConfigHelpers)

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

if(NOT CPACK_BUILD_CONFIG)
    set(CPACK_BUILD_CONFIG Release)
endif()

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
