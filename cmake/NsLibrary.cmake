# ns_add_library(<target> <sources>...)
#
# One of the numstore static libraries. Every library is flat: its headers sit
# next to its sources, and its own directory is its include directory. A
# library that links to it gets that directory too, so the include directories
# of a target are the libraries it depends on.
#
# Only ns_numstore has a public header. The headers of the rest are private to
# the build, so their include directory is BUILD_INTERFACE only.
function(ns_add_library target)
    add_library(${target} STATIC ${ARGN})
    add_library(numstore::${target} ALIAS ${target})

    target_include_directories(
        ${target}
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )
    set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    if(EXPERIMENTAL_HIERARCHIAL_TXNS)
        target_compile_definitions(
            ${target}
            PRIVATE EXPERIMENTAL_HIERARCHIAL_TXNS
        )
    endif()

    # MSVC needs experimental for c11 atomics
    if(MSVC)
        target_compile_options(${target} PUBLIC /experimental:c11atomics)
    endif()

    # A static library needs the libraries it links to at the consumer's link
    # step, so all of them are installed and exported together
    install(
        TARGETS ${target}
        EXPORT numstoreTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endfunction()
