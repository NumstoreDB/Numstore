################### Auto Generation
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/templates/smfile.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/smfile.h"
    @ONLY
)

################### BASE SOURCES
file(GLOB_RECURSE SMFILE_SOURCES  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

################### THE MAIN LIBRARY
add_library(smartfiles STATIC ${SMFILE_SOURCES})
target_include_directories(smartfiles
    PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/libs/numstore/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/libs/c_specx/include"
)
target_link_libraries(smartfiles PUBLIC c_specx numstore) 

################### INSTALL
set_target_properties(smartfiles PROPERTIES
    OUTPUT_NAME "smartfiles"
    VERSION     "${PROJECT_VERSION}"
)
install(TARGETS smartfiles
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT development
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT   development
    FILES_MATCHING PATTERN "*.h"
)
