file(GLOB_RECURSE SAMPLES_SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/smfile/*.c")

foreach(APP_SRC ${SAMPLES_SRCS})
    get_filename_component(APP_NAME ${APP_SRC} NAME_WE)
    add_executable(${APP_NAME} ${APP_SRC})
    target_link_libraries(${APP_NAME} PRIVATE c_specx nscore smartfiles numstore)
    target_include_directories(${APP_NAME} PUBLIC
        PRIVATE "${CMAKE_SOURCE_DIR}/libs/c_specx/include"
                "${CMAKE_SOURCE_DIR}/libs/nscore/include"
                "${CMAKE_SOURCE_DIR}/libs/smartfiles/include"
                "${CMAKE_SOURCE_DIR}/libs/numstore/include"
    )
endforeach()
