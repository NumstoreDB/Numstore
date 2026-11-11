# One directory per library, in dependency order. Each one defines its own
# target and links to the libraries it includes from
include(${PROJECT_SOURCE_DIR}/cmake/NsLibrary.cmake)

add_subdirectory(common)
add_subdirectory(page)
add_subdirectory(nslang)
add_subdirectory(disk_pager)
add_subdirectory(disk_wal)
add_subdirectory(nscore)
add_subdirectory(algorithms)
add_subdirectory(nsdb)
add_subdirectory(numstore)
