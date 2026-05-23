add_subdirectory(unit_test)

#################### Simple Tools
## Anything in this top level directory 
## just gets compiled and linked to all the libraries
file(
  GLOB NS_TEST_APP_SOURCES 
  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

foreach(APP_SRC ${NS_TEST_APP_SOURCES})
    get_filename_component(APP_NAME ${APP_SRC} NAME_WE)

    ######### Create and link the binary with all
    add_executable(${APP_NAME} ${APP_SRC})

    target_link_libraries(${APP_NAME} 
        PRIVATE c_specx 
                nscore 
                smartfiles 
                numstore 
                nstesting)

    target_include_directories(${APP_NAME}
        PRIVATE "${CMAKE_SOURCE_DIR}/libs/c_specx/include"
                "${CMAKE_SOURCE_DIR}/libs/nscore/include"
                "${CMAKE_SOURCE_DIR}/libs/numstore/include"
                "${CMAKE_SOURCE_DIR}/libs/smartfiles/include"
                "${CMAKE_SOURCE_DIR}/libs/nstesting/include"
    )
endforeach()
