# Renders docs/**/*.md into plain HTML with pandoc. The output directory is the
# whole site: pages mirror their source paths, style.css and assets/ sit at the
# root next to index.html.

find_program(PANDOC_EXECUTABLE pandoc REQUIRED)

set(DOCS_HTML_DIR ${CMAKE_BINARY_DIR}/html)
set(_pandoc ${CMAKE_CURRENT_SOURCE_DIR}/pandoc)

# Recursive: the pages live in an audience tree (concepts/, integrator/,
# developers/, agents/, releases/), not flat at the docs root.
file(
    GLOB_RECURSE _docs_md
    CONFIGURE_DEPENDS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.md
)

set(_docs_out "")

foreach(rel IN LISTS _docs_md)
    set(md ${CMAKE_CURRENT_SOURCE_DIR}/${rel})

    # Mirror the source path, swapping .md for .html
    string(REGEX REPLACE "\\.md$" ".html" _rel_html ${rel})
    set(_html ${DOCS_HTML_DIR}/${_rel_html})
    get_filename_component(_html_dir ${_html} DIRECTORY)

    # The pages carry their own h1 as their first line, so that line is the
    # <title> once the leading "# " is off it.
    file(STRINGS ${md} _title LIMIT_COUNT 1)
    string(REGEX REPLACE "^#+ +" "" _title "${_title}")

    # Everything the page points at is relative to the page, so the site works
    # under a GitHub Pages project path as well as at a domain root. The depth
    # picks the matching sidebar and prefixes style.css and the logo.
    string(REGEX MATCHALL "/" _slashes "${rel}")
    list(LENGTH _slashes _depth)
    set(_up "")
    if(_depth GREATER 0)
        foreach(_i RANGE 1 ${_depth})
            string(APPEND _up "../")
        endforeach()
    endif()

    add_custom_command(
        OUTPUT ${_html}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${_html_dir}
        COMMAND
            ${PANDOC_EXECUTABLE} ${md} --output ${_html} --standalone --from=gfm
            --to=html5 --template=${_pandoc}/template.html
            --lua-filter=${_pandoc}/md-links.lua
            --include-before-body=${_pandoc}/_sidebar${_depth}.html
            --css=${_up}style.css --metadata=pagetitle=${_title}
            --metadata=siteroot=${_up}
        DEPENDS
            ${md}
            ${_pandoc}/template.html
            ${_pandoc}/md-links.lua
            ${_pandoc}/_sidebar${_depth}.html
        COMMENT "pandoc ${rel}"
        VERBATIM
    )

    list(APPEND _docs_out ${_html})
endforeach()

# Static files the pages reference.
add_custom_command(
    OUTPUT ${DOCS_HTML_DIR}/style.css
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different ${_pandoc}/style.css
        ${DOCS_HTML_DIR}/style.css
    COMMAND
        ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${DOCS_HTML_DIR}/assets
    DEPENDS ${_pandoc}/style.css
    COMMENT "Copying docs assets"
    VERBATIM
)

add_custom_target(docs ALL DEPENDS ${_docs_out} ${DOCS_HTML_DIR}/style.css)
