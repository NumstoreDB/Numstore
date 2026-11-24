file(GLOB NUMSTORE_SAMPLES CONFIGURE_DEPENDS *.c)

foreach(sample IN LISTS NUMSTORE_SAMPLES)
    get_filename_component(name ${sample} NAME_WE)
    add_executable(${name} ${sample})
    target_link_libraries(${name} PRIVATE ns_numstore)
endforeach()
