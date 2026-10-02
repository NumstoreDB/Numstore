# docs/CMakeLists.txt
#
# Builds docs/**/*.md -> <build>/docs/html/**/*.html with pandoc,
# and installs the HTML plus the raw markdown under share/doc/numstore.

find_program(PANDOC_EXE NAMES pandoc)

if(NOT PANDOC_EXE)
    message(WARNING "pandoc not found; docs will not be built")
    return()
endif()

set(_pandoc_dir ${CMAKE_CURRENT_SOURCE_DIR}/pandoc)

set(_pandoc_deps
    ${_pandoc_dir}/md-links.lua
    ${_pandoc_dir}/style.css
    ${_pandoc_dir}/template.html
    ${_pandoc_dir}/_sidebar.html
)

set(_pandoc_args
    --from=markdown
    --to=html5
    --standalone
    --embed-resources
    --lua-filter=${_pandoc_dir}/md-links.lua
    --css=${_pandoc_dir}/style.css
    --template=${_pandoc_dir}/template.html
    --include-before-body=${_pandoc_dir}/_sidebar.html
)

# ---- collect markdown (everything except docs/pandoc/) ----
file(GLOB_RECURSE _md_files CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.md)

set(_html_dir ${CMAKE_CURRENT_BINARY_DIR}/html)
set(_html_outputs)

foreach(md IN LISTS _md_files)
    file(RELATIVE_PATH _rel ${CMAKE_CURRENT_SOURCE_DIR} ${md}) # foo/bar.md
    if(_rel MATCHES "^pandoc/")
        continue()
    endif()

    string(REGEX REPLACE "\\.md$" ".html" _rel_html "${_rel}")
    set(_out ${_html_dir}/${_rel_html})
    get_filename_component(_out_dir ${_out} DIRECTORY)

    add_custom_command(
        OUTPUT ${_out}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${_out_dir}
        COMMAND
            ${CMAKE_COMMAND} -E env PANDOC_SRC_REL=${_rel} ${PANDOC_EXE}
            ${_pandoc_args} --output ${_out} ${md}
        DEPENDS ${md} ${_pandoc_deps}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "pandoc ${_rel}"
        VERBATIM
    )

    list(APPEND _html_outputs ${_out})
endforeach()

add_custom_target(docs ALL DEPENDS ${_html_outputs})

install(DIRECTORY ${_html_dir}/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/html)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_DOCDIR}/markdown
    FILES_MATCHING
    PATTERN "*.md"
    PATTERN "pandoc" EXCLUDE
)
