file(GLOB_RECURSE 
  NSPY_SOURCES 
  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

Python_add_library(_numstore MODULE WITH_SOABI module.c ${NSPY_SOURCES})

# Save include paths for other targets to include from
set(CPYNS_PUBLIC_INCLUDE  CACHE PATH "")
set(CPYNS_PRIVATE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/src" CACHE PATH "")

target_include_directories(_numstore
    PUBLIC          "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PUBLIC          "${CMAKE_CURRENT_SOURCE_DIR}/src"
                    "${CMAKE_SOURCE_DIR}/libs/numstore/src" # TODO - fix this
)

# Include Numpy includes seperately to ignore compiler flags
target_include_directories(_numstore
    SYSTEM PRIVATE  "${NumPy_INCLUDE}"
)

target_link_libraries(_numstore PRIVATE c_specx nscore numstore)

install(TARGETS _numstore DESTINATION numstore)
