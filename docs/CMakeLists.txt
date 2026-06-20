########################################################################
# Find all dependencies
########################################################################
find_program(PANDOC_EXECUTABLE pandoc DOC "Path to the pandoc executable")
if(NOT PANDOC_EXECUTABLE)
    message(FATAL_ERROR "pandoc not found. Install it or set PANDOC_EXECUTABLE.")
endif()
message(STATUS "Using pandoc: ${PANDOC_EXECUTABLE}")

########################################################################
# Configuration
########################################################################
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/html")

# Glob relative to the source dir so we can map md -> html easily
file(GLOB_RECURSE MD_FILES
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    CONFIGURE_DEPENDS
    "*.md")

set(PANDOC_LUA      "${PROJECT_SOURCE_DIR}/docs/pandoc/md-links.lua")
set(PANDOC_CSS      "${PROJECT_SOURCE_DIR}/docs/pandoc/style.css")
set(PANDOC_TEMPLATE "${PROJECT_SOURCE_DIR}/docs/pandoc/template.html")
set(PANDOC_SIDEBAR  "${PROJECT_SOURCE_DIR}/docs/pandoc/_sidebar.html")

set(PANDOC_ARGS
    --from=markdown
    --to=html5
    --standalone
    --embed-resources
    --lua-filter=${PANDOC_LUA}
    --css=${PANDOC_CSS}
    --template=${PANDOC_TEMPLATE}
    --include-before-body=${PANDOC_SIDEBAR}
  )

########################################################################
# Gather all MD Sources
########################################################################
set(HTML_OUTPUTS "")
foreach(MD_REL IN LISTS MD_FILES)
  
  # Absolute path
  set(MD_ABS "${CMAKE_CURRENT_SOURCE_DIR}/${MD_REL}")

  # Replace .md with .html
  string(REGEX REPLACE "\\.md$" ".html" HTML_REL "${MD_REL}")

  # Output path
  set(HTML_ABS "${CMAKE_BINARY_DIR}/html/${HTML_REL}")

  get_filename_component(HTML_SUBDIR "${HTML_ABS}" DIRECTORY)
  add_custom_command(
    OUTPUT  "${HTML_ABS}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${HTML_SUBDIR}"
    COMMAND ${PANDOC_EXECUTABLE} ${PANDOC_ARGS}
                --output "${HTML_ABS}"
                "${MD_ABS}"
    DEPENDS "${MD_ABS}" 
            "${PANDOC_CSS}" 
            "${PANDOC_TEMPLATE}"
            "${PANDOC_LUA}"
            "${PANDOC_SIDEBAR}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "pandoc: ${MD_REL} -> ${HTML_REL}"
    VERBATIM
  )
  list(APPEND HTML_OUTPUTS "${HTML_ABS}")
endforeach()

########################################################################
# Copy image/asset files next to the HTML output
########################################################################
file(GLOB_RECURSE ASSET_FILES
    RELATIVE assets
    CONFIGURE_DEPENDS
    assets/*.png
    assets/*.jpg
    assets/*.jpeg
    assets/*.gif
    assets/*.svg
    assets/*.webp
  )

foreach(ASSET_REL IN LISTS ASSET_FILES)
    set(ASSET_SRC assets/${ASSET_REL})
    set(ASSET_DST "${CMAKE_BINARY_DIR}/html/assets/${ASSET_REL}")
    get_filename_component(ASSET_SUBDIR "${ASSET_DST}" DIRECTORY)

    add_custom_command(
        OUTPUT  "${ASSET_DST}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ASSET_SUBDIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ASSET_SRC}" "${ASSET_DST}"
        DEPENDS "${ASSET_SRC}"
        COMMENT "Copying asset: ${ASSET_REL}"
        VERBATIM
    )
    list(APPEND HTML_OUTPUTS "${ASSET_DST}")
endforeach()

########################################################################
# Aggregate target + install
########################################################################
add_custom_target(docs ALL
    DEPENDS ${HTML_OUTPUTS}
    COMMENT "Building HTML documentation"
)

install(DIRECTORY "${CMAKE_BINARY_DIR}/html/"
    DESTINATION share/doc/${PROJECT_NAME}
)
