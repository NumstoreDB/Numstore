################### Auto Generation
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/templates/compile_config.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/smfile/compile_config.h"
    @ONLY
)

################### BASE SOURCES
file(
  GLOB_RECURSE SMFILE_SOURCES  
  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

################### THE MAIN LIBRARY
add_library(smartfiles STATIC ${SMFILE_SOURCES})
target_include_directories(smartfiles
    PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
            "${CMAKE_SOURCE_DIR}/libs/nscore/include"
            "${CMAKE_SOURCE_DIR}/libs/c_specx/include"
)
target_link_libraries(smartfiles PUBLIC c_specx nscore) 
