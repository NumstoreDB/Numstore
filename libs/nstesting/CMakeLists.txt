file(GLOB TESTING_SOURCES CONFIGURE_DEPENDS "src/*.c")

add_library(nstesting STATIC ${TESTING_SOURCES})

target_include_directories(nstesting
  PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
          "${CMAKE_SOURCE_DIR}/libs/numstore/src" # TODO - fix this
)

target_link_libraries(nstesting
    PRIVATE c_specx 
            nscore 
            smartfiles 
            numstore
)
