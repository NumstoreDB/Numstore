#################### Simple Tools
## Anything in this top level directory 
## just gets compiled and linked to all the libraries
file(
  GLOB NS_SIMPLE_APP_SOURCES 
  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

foreach(APP_SRC ${NS_SIMPLE_APP_SOURCES})
    get_filename_component(APP_NAME ${APP_SRC} NAME_WE)

    ######### Create and link the binary with all
    add_executable(${APP_NAME} ${APP_SRC})
    target_link_libraries(${APP_NAME} PRIVATE c_specx nscore smartfiles)

    ######### Install 
    install(TARGETS ${APP_NAME}
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT runtime
    )
endforeach()
