set(NS_VERSION_MAJOR  ${PROJECT_VERSION_MAJOR})
set(NS_VERSION_MINOR  ${PROJECT_VERSION_MINOR})
set(NS_VERSION_PATCH  ${PROJECT_VERSION_PATCH})
set(NS_VERSION_STRING "${PROJECT_VERSION}")

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

set_target_properties(numstore PROPERTIES VERSION ${PROJECT_VERSION})
if(NOT WIN32)
    set_target_properties(numstore PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
endif()

target_include_directories(numstore PRIVATE "${PROJECT_SOURCE_DIR}/src")

# Link to systems thread library
if(WIN32)
    target_link_libraries(numstore PUBLIC ws2_32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(numstore PUBLIC Threads::Threads)
endif()
