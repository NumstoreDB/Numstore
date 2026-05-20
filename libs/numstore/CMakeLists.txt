################### Auto Generation
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/templates/numstore.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/numstore.h"
    @ONLY
)

################### BASE SOURCES
file(GLOB_RECURSE NS_SOURCES  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

################### THE MAIN LIBRARY
add_library(numstore STATIC ${NS_SOURCES})
target_include_directories(numstore
    PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
            "${CMAKE_SOURCE_DIR}/libs/nscore/include"
            "${CMAKE_SOURCE_DIR}/libs/c_specx/include"
)
target_link_libraries(numstore PUBLIC c_specx nscore) 

################### INSTALL
set_target_properties(numstore PROPERTIES
    OUTPUT_NAME "numstore"
    VERSION     "${PROJECT_VERSION}"
)
install(TARGETS numstore
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT development
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT   development
    FILES_MATCHING PATTERN "*.h"
)
