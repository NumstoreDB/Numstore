# Optionally Add NDEBUG macro
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INSTALL_DO_STRIP TRUE)
    add_compile_definitions(NDEBUG)
endif()

if(ENABLE_TESTS)
  add_compile_definitions(TESTING)
endif()

# Optionally disable logs
if(NOT ENABLE_LOGGING)
  add_compile_definitions(NLOG)
endif()

if(MSVC)
    include(msvc_compiler_flags)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    include(clang_compiler_flags)
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU")
    include(gcc_compiler_flags)
else()
    message(WARNING "Unknown compiler: ${CMAKE_C_COMPILER_ID}. No custom flags applied.")
endif()
