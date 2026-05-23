################### Meta Information
cmake_minimum_required(VERSION 3.18)
file(STRINGS "${CMAKE_SOURCE_DIR}/version.txt" NS_VERSION LIMIT_COUNT 1)
project(numstore
    VERSION     "${NS_VERSION}"
    DESCRIPTION "Custom database engine for arrays"
    LANGUAGES   C
)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

################### Common Variables for Code Gen
include(ProcessorCount)
ProcessorCount(NS_CPU_COUNT)
if(NS_CPU_COUNT EQUAL 0)
    set(NS_CPU_COUNT 1)  
endif()
set(NS_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(NS_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(NS_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(NS_VERSION_STRING "${PROJECT_VERSION}")
set(NS_PAGE_SIZE        4096    CACHE STRING "Page size")
set(NS_MEMORY_PAGE_LEN  4096    CACHE STRING "Number of buffer pool slots")
set(NS_WAL_BUFFER_CAP   1048576 CACHE STRING "WAL buffer capacity in bytes")
set(TSIZE_BITS          32      CACHE STRING "Index into a type")
set(PSIZE_BITS          32      CACHE STRING "Index into a page")
set(BSIZE_BITS          64      CACHE STRING "Index into a byte stream or array")
set(PGNO_BITS           64      CACHE STRING "Page number")
set(TXID_BITS           64      CACHE STRING "Transaction id")
set(LSN_BITS            64      CACHE STRING "WAL log sequence number")
set(PGH_BITS            8       CACHE STRING "Page header size")
set(WLH_BITS            8       CACHE STRING "WAL header size")

################### Tools
include(GNUInstallDirs)
include(cmake/Options.cmake)
include(cmake/CompilerFlags.cmake)
include(cmake/Sanitizers.cmake)
include(cmake/Packaging.cmake)
include(cmake/Python.cmake)

################### Inner Projects
add_subdirectory(libs/c_specx)
add_subdirectory(libs/nscore)
add_subdirectory(libs/numstore)
add_subdirectory(libs/smartfiles)
add_subdirectory(libs/nstesting)
#add_subdirectory(libs/pynumstore)
add_subdirectory(apps)
add_subdirectory(samples)
