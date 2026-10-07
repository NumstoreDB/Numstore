# Copyright 2026 Theo Lincke
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Builds numstore.jar (Java API) and libnumstore_jni (JNI glue).
#
# Standalone:
#   cmake -B build -DNUMSTORE_INCLUDE_DIR=/path/to/include \
#                  -DNUMSTORE_LIBRARY=/path/to/libnumstore.so
#   cmake --build build
#
# Inside the numstore tree: add_subdirectory(java) after the `numstore`
# library target is defined and it is linked automatically.
#
# A static libnumstore.a also works, as long as it was compiled with -fPIC.

cmake_minimum_required(VERSION 3.16)
project(numstore_java C Java)

find_package(Java 11 REQUIRED COMPONENTS Development)
find_package(JNI REQUIRED)
include(UseJava)

set(NUMSTORE_INCLUDE_DIR "" CACHE PATH "Directory containing numstore.h")
set(NUMSTORE_LIBRARY
    ""
    CACHE FILEPATH
    "numstore library to link (ignored if a 'numstore' target exists)"
)

# ------------------------------------------------------------ Java

set(CMAKE_JAVA_COMPILE_FLAGS --release 11 -Xlint:all)
file(
    GLOB NUMSTORE_JAVA_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/java/com/numstore/*.java
)

add_jar(
    numstore_jar
    SOURCES ${NUMSTORE_JAVA_SOURCES}
    OUTPUT_NAME numstore
    GENERATE_NATIVE_HEADERS numstore_jni_headers
)

# ------------------------------------------------------------ JNI library

add_library(numstore_jni SHARED src/main/c/numstore_jni.c)
target_include_directories(numstore_jni PRIVATE ${JNI_INCLUDE_DIRS})
target_link_libraries(numstore_jni PRIVATE numstore_jni_headers)
set_target_properties(
    numstore_jni
    PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(numstore_jni PRIVATE -Wall -Wextra)
endif()

if(TARGET numstore)
    target_link_libraries(numstore_jni PRIVATE numstore)
else()
    if(NOT NUMSTORE_INCLUDE_DIR OR NOT NUMSTORE_LIBRARY)
        message(
            FATAL_ERROR
            "Set NUMSTORE_INCLUDE_DIR and NUMSTORE_LIBRARY, or add this directory "
            "from a project that defines a 'numstore' target."
        )
    endif()
    target_include_directories(numstore_jni PRIVATE ${NUMSTORE_INCLUDE_DIR})
    target_link_libraries(numstore_jni PRIVATE ${NUMSTORE_LIBRARY})
endif()
