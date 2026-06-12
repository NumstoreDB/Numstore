option(ENABLE_NDEBUG           "Disable assertions (NDEBUG)"                OFF)
option(ENABLE_NTEST            "Exclude test code at compile time (NTEST)"  OFF)
option(ENABLE_NLOG             "Disable all logging (NLOG)"                 OFF)
option(ENABLE_COVERAGE         "Enable coverage for tests"                  OFF)
option(ENABLE_PORTABLE         "Portable build: no -march=native"           OFF)
option(ENABLE_STRIP            "Strip debug symbols from installed binary"  OFF)
option(ENABLE_ASAN             "Enable AddressSanitizer"                    OFF)

# Enable / Disable Certain Artifacts
option(BUILD_TOOLS             "Build small useful cli tools for numstore"  ON )
option(BUILD_SAMPLES           "Build samples"                              ON )

# Variables
set(LOG_LEVEL "LOG_INFO" CACHE STRING "Set the log level (ignored if NLOG)")

######### Save as preprocessor macros

if(ENABLE_NDEBUG)
  add_compile_definitions(NDEBUG)
endif()

if(ENABLE_NTEST)
  add_compile_definitions(NTEST)
endif()

if(ENABLE_NLOG)
  add_compile_definitions(NLOG)
endif()

if(LOG_LEVEL)
  add_compile_definitions(I_LOG_LEVEL=${LOG_LEVEL})
endif()

######### Add coverage flags to the compiler

if(ENABLE_COVERAGE)
    add_compile_options(--coverage -fprofile-update=atomic)
    add_link_options(--coverage)
endif()

######### Add address sanitizer flags to the compiler

if(ENABLE_ASAN)
    if(MSVC)
        add_compile_options(/fsanitize=address /Zi) 
        add_link_options(/INCREMENTAL:NO) 
    else()
        # Linux/macOS Flags: Use -fsanitize=address
        # -fno-omit-frame-pointer: Ensures nice, readable stack traces
        add_compile_options(
          -fsanitize=address          # Add address sanitizer
          -fno-omit-frame-pointer     # Readable stack traces
          -g                          # Keep symbols
        )
        
        # Linker must also have the flag to pull in the ASan library
        add_link_options(-fsanitize=address)

        # Add leak sanitizer for linux too
        # Apple disables it - because they use the apple command leak
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            add_compile_options(-fsanitize=leak)
            add_link_options(-fsanitize=leak)
        endif()
    endif()
endif()
