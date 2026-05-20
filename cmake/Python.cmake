set(Python_FIND_VIRTUALENV STANDARD)
find_package(Python COMPONENTS Interpreter REQUIRED)

# add_python_subdirectory(TARGET_NAME SOURCE_DIR)
#
# TARGET_NAME: The name of the CMake target to create (e.g., pynumstore_wheel)
# SOURCE_DIR:  The relative path to the Python package directory containing pyproject.toml
function(add_python_subdirectory TARGET_NAME SOURCE_DIR)
    # Define where the wheel should be output in the CMake build directory
    set(WHEEL_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_dist")

    # Create a custom target that runs the Python build process
    # ALL means it will build automatically when you run `cmake --build .`
    add_custom_target(${TARGET_NAME} ALL
        COMMAND ${Python_EXECUTABLE} -m build 
            --wheel 
            --outdir "${WHEEL_OUT_DIR}" 
            "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}"
        COMMENT "Building Python wheel for ${SOURCE_DIR} into ${WHEEL_OUT_DIR}..."
        USES_TERMINAL # Streams the build output directly to the console
    )

    # Ensure the wheel output directory is cleaned up during `make clean`
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES "${WHEEL_OUT_DIR}")
endfunction()
