file(
  GLOB NUMSTORE_SRCS CONFIGURE_DEPENDS 
  "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

foreach(APP_SRC ${NUMSTORE_SRCS})
    get_filename_component(APP_NAME ${APP_SRC} NAME_WE)
    add_executable(${APP_NAME} ${APP_SRC})
    target_link_libraries(${APP_NAME} PRIVATE numstore)
    target_include_directories(${APP_NAME} 
        PRIVATE "${CMAKE_SOURCE_DIR}/libs/numstore/include"
    )
endforeach()
