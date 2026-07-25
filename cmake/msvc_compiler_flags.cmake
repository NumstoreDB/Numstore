add_compile_options(
  /W3
  /wd4100 
  /wd4101 
  /wd4244 
  /wd4267
  /experimental:c11atomics
)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(/Od /Zi)
endif()

if(ENABLE_ASAN)
    add_compile_options(/fsanitize=address /Zi)
    add_link_options(/INCREMENTAL:NO)
endif()

if(NOT ENABLE_PORTABLE)
    add_compile_options(/arch:AVX2)
endif()

if(ENABLE_COVERAGE)
    message(FATAL_ERROR "Coverage is only supported on GCC/Clang.")
endif()
