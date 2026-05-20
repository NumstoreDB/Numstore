################### Auto Generation
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/templates/compile_config.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/nscore/compile_config.h"
    @ONLY
)

################### BASE SOURCES
file(GLOB_RECURSE NS_SOURCES  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

################### THE MAIN LIBRARY
add_library(nscore STATIC ${NS_SOURCES})
target_include_directories(nscore
    PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/src"       
            "${CMAKE_SOURCE_DIR}/libs/c_specx/include"
)
target_link_libraries(nscore PRIVATE c_specx)

################### INSTALL
set_target_properties(nscore PROPERTIES
    OUTPUT_NAME "nscore"
    VERSION     "${PROJECT_VERSION}"
)
install(TARGETS nscore
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT development
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT   development
    FILES_MATCHING PATTERN "*.h"
)
