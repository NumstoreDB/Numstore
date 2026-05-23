file(GLOB TESTING_SOURCES CONFIGURE_DEPENDS "src/*.c")

add_library(nstesting STATIC ${TESTING_SOURCES})

target_include_directories(nstesting
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include"
          "${CMAKE_CURRENT_SOURCE_DIR}/src"

          "${CMAKE_SOURCE_DIR}/libs/c_specx/include"
          "${CMAKE_SOURCE_DIR}/libs/nscore/include"
          "${CMAKE_SOURCE_DIR}/libs/numstore/include"
          "${CMAKE_SOURCE_DIR}/libs/numstore/src"
          "${CMAKE_SOURCE_DIR}/libs/smartfiles/include"
)

target_link_libraries(nstesting
    PRIVATE c_specx 
            nscore 
            smartfiles 
            numstore
)
