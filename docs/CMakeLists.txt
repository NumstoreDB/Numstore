# Renders docs/**/*.md into plain HTML with pandoc. The output directory is the
# whole site: pages keep their folders, style.css and assets/ are at the root.

find_program(PANDOC_EXECUTABLE pandoc REQUIRED)

set(DOCS_HTML_DIR ${CMAKE_BINARY_DIR}/html)
set(_pandoc ${CMAKE_CURRENT_SOURCE_DIR}/pandoc)

file(GLOB_RECURSE _docs_md CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.md)

# Release paperwork is for whoever cuts the release, not for the site.
list(
    REMOVE_ITEM _docs_md
    ${CMAKE_CURRENT_SOURCE_DIR}/release.md
    ${CMAKE_CURRENT_SOURCE_DIR}/moc_release.md
)

set(_docs_out "")

foreach(md IN LISTS _docs_md)
    file(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${md})
    get_filename_component(rel_dir ${rel} DIRECTORY)
    string(REGEX REPLACE "\\.md$" ".html" rel_html ${rel})
    set(_html ${DOCS_HTML_DIR}/${rel_html})

    # One "../" for each folder between the page and the site root.
    set(up "")
    if(rel_dir)
        string(REGEX REPLACE "[^/]+" ".." up ${rel_dir})
        set(up "${up}/")
    endif()

    # The pages carry their own h1, so the first line is the <title> only.
    file(STRINGS ${md} _title LIMIT_COUNT 1)

    # The template pulls in sidebar.html as a partial; root prefixes its links.
    add_custom_command(
        OUTPUT ${_html}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DOCS_HTML_DIR}/${rel_dir}
        COMMAND
            ${PANDOC_EXECUTABLE} ${md} --output ${_html} --standalone --from=gfm
            --to=html5 --template=${_pandoc}/template.html
            --lua-filter=${_pandoc}/md-links.lua --css=${up}style.css
            --variable=root:${up} --metadata=pagetitle=${_title}
        DEPENDS
            ${md}
            ${_pandoc}/template.html
            ${_pandoc}/md-links.lua
            ${_pandoc}/sidebar.html
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
