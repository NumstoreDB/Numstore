file(GLOB_RECURSE 
  NSPY_SOURCES 
  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/c/src/*.c"
)

Python_add_library(_numstore MODULE WITH_SOABI c/module.c ${NSPY_SOURCES})

# Save include paths for other targets to include from
set(CPYNS_PUBLIC_INCLUDE  CACHE PATH "")
set(CPYNS_PRIVATE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/c/src" CACHE PATH "")

target_include_directories(_numstore
    PUBLIC          "${CMAKE_CURRENT_SOURCE_DIR}/c/include"
    PUBLIC          "${CMAKE_CURRENT_SOURCE_DIR}/c/src"
                    "${CMAKE_SOURCE_DIR}/libs/numstore/src" # TODO - fix this
)

# Include Numpy includes seperately to ignore compiler flags
target_include_directories(_numstore
    SYSTEM PRIVATE  "${NumPy_INCLUDE}"
)

if(MSVC)
    set_property(TARGET _numstore
      PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
    )
endif()

target_link_libraries(_numstore PRIVATE c_specx nscore numstore)

install(TARGETS _numstore DESTINATION numstore)
