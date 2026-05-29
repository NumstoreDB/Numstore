file(
  GLOB SAMPLES_SRCS 
  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

# Compile each sample
foreach(APP_SRC ${SAMPLES_SRCS})
    get_filename_component(APP_NAME ${APP_SRC} NAME_WE)
    add_executable(${APP_NAME} ${APP_SRC})

    target_link_libraries(${APP_NAME} PRIVATE smartfiles)
endforeach()
