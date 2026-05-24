if(MSVC)
    add_compile_options(
      # Include all warnings
      /W3                       
      
      # Unreferenced formal parameter
      /wd4100                   
      
      # Unused local variable
      /wd4101                   
      
      # Convert - possible loss of data
      /wd4244                   
      
      # Convert from size_t to type
      /wd4267                   
      
      # Support C11 atomic operations
      /experimental:c11atomics  
    )

    # For debug build:
    #   - Disable optimizations (equivalent to -O0)
    #   - Create Symbols (equivalent to -g)
    add_compile_options("$<$<CONFIG:Debug>:/Od;/Zi>")
else()
    add_compile_options(
        # Enable standard warnings
        -Wall                         

        # Enable even more warnings
        -Wextra                       

        # Treat all warnings as errors
        -Werror                       

        # Warn when a variable shadows another
        -Wshadow                      

        # Warn on signed/unsigned comparisons
        -Wsign-compare                

        # Force function prototypes in C
        -Wstrict-prototypes           

        # Warn if global function lacks prototype
        -Wmissing-prototypes          

        # Warn if global function lacks declaration
        -Wmissing-declarations        

        # Strict ISO C/C++ compliance
        -pedantic-errors              

        # Silence "unused parameter" warnings
        -Wno-unused-parameter         

        # Silence "unused variable" warnings
        -Wno-unused-variable          

        # Silence "assigned but never used" warnings
        -Wno-unused-but-set-variable 
    )

    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        add_compile_options(
          # I use __VA_ARGS__ a lot in TEST and LOG 
          # - I might be able to refactor this but for now disable it
          -Wno-gnu-zero-variadic-macro-arguments 

          # Silence Clang warnings on static inline functions
          -Wno-static-in-inline                  
        )
    endif()

    # For debug build:
    #   - Disable Optimizations
    #   - Create symbols
    add_compile_options("$<$<CONFIG:Debug>:-O0;-g>") 

    if(ENABLE_PORTABLE)
        # Tune output to baseline 64 bit or armv8 
        # standard and don't use newer features 
        # like AVX-512
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
            add_compile_options(-march=x86-64 -mtune=generic) 
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
            add_compile_options(-march=armv8-a -mtune=generic) 
        endif()
    else()
        # You can tune this build for this specific machine
        add_compile_options(-march=native -mtune=native) 
    endif()
endif()

# Enable profiling
if(ENABLE_GPROF)
  if(MSVC)
    add_link_options(/PROFILE)
  else()
    add_compile_options(-pg)    
    add_link_options(-pg)      
  endif()
endif()

# Strip symbols
if(ENABLE_STRIP)
    set(CMAKE_INSTALL_DO_STRIP TRUE) 
endif()
