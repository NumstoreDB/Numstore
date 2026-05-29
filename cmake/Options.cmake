option(ENABLE_NDEBUG           "Disable assertions (NDEBUG)"                OFF)
option(ENABLE_NTEST            "Exclude test code at compile time (NTEST)"  OFF)
option(ENABLE_NLOG             "Disable all logging (NLOG)"                 OFF)
option(ENABLE_GPROF            "Enable gprof profiling support"             OFF)
option(ENABLE_COVERAGE         "Enable coverage for tests"                  OFF)
option(ENABLE_PORTABLE         "Portable build: no -march=native"           OFF)
option(ENABLE_STRIP            "Strip debug symbols from installed binary"  OFF)
option(ENABLE_ASAN             "Enable AddressSanitizer"                    OFF)
option(BUILD_APPS              "Build apps"                                 ON )
option(BUILD_BINDINGS          "Build bindings"                             ON )
option(BUILD_SAMPLES           "Build samples"                              ON )
option(BUILD_NUMSTORE_SAMPLES  "Build numstore samples"                     OFF)
option(BUILD_SMARTFILE_SAMPLES "Build smart file samples"                   ON )
option(ENABLE_LEAK_TOOL        "Enables leak tool (valgrind or leaks)"      OFF)

# Translates ENABLE_* options into preprocessor defines.
foreach(_flag NDEBUG NTEST NLOG)
    if(ENABLE_${_flag})
        add_compile_definitions(${_flag})
    endif()
endforeach()

if(ENABLE_COVERAGE)
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()

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

