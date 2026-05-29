################### Auto Generation
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/templates/compile_config.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/nscore/compile_config.h"
    @ONLY
)

################### BASE SOURCES
file(
  GLOB_RECURSE NS_SOURCES  
  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

################### THE MAIN LIBRARY
add_library(nscore STATIC ${NS_SOURCES})
target_include_directories(nscore
  PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(nscore PRIVATE c_specx)
