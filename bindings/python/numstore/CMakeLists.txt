file(GLOB_RECURSE 
  NSPY_SOURCES 
  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/c/src/*.c"
)

Python_add_library(_numstore MODULE WITH_SOABI ${NSPY_SOURCES})

target_include_directories(_numstore
    PUBLIC          "${CMAKE_CURRENT_SOURCE_DIR}/c/include"
    PRIVATE         "${CMAKE_CURRENT_SOURCE_DIR}/c/src"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../nscore/include"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../c_specx/include"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../numstore/include"
)

# Include Numpy includes seperately to ignore compiler flags
target_include_directories(_numstore
    SYSTEM PRIVATE  "${NumPy_INCLUDE_DIRS}"
)

target_link_libraries(_numstore PRIVATE c_specx nscore numstore)

install(TARGETS _numstore DESTINATION numstore)
