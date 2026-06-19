########################################################################
# Generate Compile Time Config
#     These are all the configurable compile time constants 
#     in numstore - things like the number of bits per data element 
#     PAGE_SIZE, memory pool size etc.
########################################################################
include(ProcessorCount)
ProcessorCount(NS_CPU_COUNT)
if(NS_CPU_COUNT EQUAL 0)
    set(NS_CPU_COUNT 1)
endif()

set(NS_VERSION_MAJOR  ${PROJECT_VERSION_MAJOR})
set(NS_VERSION_MINOR  ${PROJECT_VERSION_MINOR})
set(NS_VERSION_PATCH  ${PROJECT_VERSION_PATCH})
set(NS_VERSION_STRING "${PROJECT_VERSION}")

set(NS_PAGE_SIZE       4096    CACHE STRING "Page size")
set(NS_MAX_OPEN_FILES  20      CACHE STRING "Maximum number of open files")
set(NS_MEMORY_PAGE_LEN 4096    CACHE STRING "Number of buffer pool slots")
set(NS_WAL_BUFFER_CAP  1048576 CACHE STRING "WAL buffer capacity in bytes")

set(TSIZE_BITS 32 CACHE STRING "Index into a type")
set(PSIZE_BITS 32 CACHE STRING "Index into a page")
set(BSIZE_BITS 64 CACHE STRING "Index into a byte stream or array")
set(PGNO_BITS  64 CACHE STRING "Page number")
set(TXID_BITS  64 CACHE STRING "Transaction id")
set(LSN_BITS   64 CACHE STRING "WAL log sequence number")
set(PGH_BITS   8  CACHE STRING "Page header size")
set(WLH_BITS   8  CACHE STRING "WAL header size")

configure_file(
    "${PROJECT_SOURCE_DIR}/src/templates/compile_config.h.in"
    "${PROJECT_SOURCE_DIR}/src/compile_config.h"
    @ONLY
)

########################################################################
# Generate the numstore library 
########################################################################

# Glob all *.c files together
file(GLOB NUMSTORE_SOURCES CONFIGURE_DEPENDS "*.c")
file(GLOB NUMSTORE_TESTING_SOURCES CONFIGURE_DEPENDS "testing/*.c")

# Add test specific code if we're compiling tests
if(ENABLE_TESTS)
    add_library(numstore ${NUMSTORE_SOURCES} ${NUMSTORE_TESTING_SOURCES})
else()
    add_library(numstore ${NUMSTORE_SOURCES})
endif()

add_library(numstore::numstore ALIAS numstore)

set_target_properties(numstore PROPERTIES 
    VERSION   ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(numstore PRIVATE "${PROJECT_SOURCE_DIR}/src")

# Link to systems thread library
if(WIN32)
    target_link_libraries(numstore PUBLIC ws2_32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(numstore PUBLIC Threads::Threads)
endif()
