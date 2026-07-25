add_compile_options(
    -Wall 
    -Wextra 
    -Werror
    -Wshadow 
    -Wsign-compare
    -Wstrict-prototypes 
    -Wmissing-prototypes 
    -Wmissing-declarations
    -pedantic-errors
    -Wno-unused-parameter 
    -Wno-unused-variable 
    -Wno-unused-but-set-variable
)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(-g -O0)
else()
  add_compile_options(-O3)
endif()

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_compile_options(-fsanitize=leak)
        add_link_options(-fsanitize=leak)
    endif()
endif()

if(ENABLE_COVERAGE)
    add_compile_options(--coverage -fprofile-update=atomic)
    add_link_options(--coverage)
endif()

if(ENABLE_PORTABLE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
        add_compile_options(-march=x86-64 -mtune=generic)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
        add_compile_options(-march=armv8-a -mtune=generic)
    endif()
else()
    add_compile_options(-march=native -mtune=native)
endif()
