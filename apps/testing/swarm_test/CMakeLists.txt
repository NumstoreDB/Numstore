file(GLOB SWARM_TEST_SOURCES "*.c")

add_executable(swarm_test ${SWARM_TEST_SOURCES})

target_link_libraries(swarm_test
    PRIVATE c_specx 
            nscore 
            smartfiles 
            numstore
)

target_include_directories(swarm_test
  PRIVATE "${CMAKE_SOURCE_DIR}/libs/c_specx/include"
          "${CMAKE_SOURCE_DIR}/libs/nscore/include"

          "${CMAKE_SOURCE_DIR}/libs/numstore/include"
          "${CMAKE_SOURCE_DIR}/libs/numstore/src"

          "${CMAKE_SOURCE_DIR}/libs/smartfiles/include"
)
