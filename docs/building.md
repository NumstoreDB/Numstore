Building Numstore
=================

System requirements
-------------------

* C compiler supporting the C11 standard
* CMake
* Python 3

Linux, Mac, Windows
-------------------

Numstore is built with CMake.
Default build config is `Debug`:

    cd <path>/numstore
    mkdir build 
    cd build 
    cmake ..
    cmake --build .


Build in Release Mode
---------------------

Release mode has no tests, and no ASSERTS

    cd <path>/numstore
    mkdir build 
    cd build 
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build .

Run a sample application inside `src/samples` use these as pedagogical 
sample applications that show you how numstore works

Configure your build
====================

CMake Options
-------------

* `ENABLE_PORTABLE`              - Build code without machine optimized instructions (to distribute and valgrind)
* `ENABLE_TESTS`                 - Include testing specific code in the numstore library
* `ENABLE_LOGGING`               - Include logging code in the numstore library 
* `ENABLE_ASAN`                  - Compile in an address sanitizer
* `ENABLE_COVERAGE`              - Compile in coverage flag (for gnu only)
* `BUILD_TOOLS`                  - Build a bunch of command line tools
* `BUILD_SAMPLES`                - Build the sample code

Configure Compile Time attributes 
---------------------------------

Numstore has compile time configuration of various application specific attributes
such as page size, number of bits for a page number etc.

You can see them all in cmake/Config.cmake.

These are common configuration options:

* `-DNS_NS_PAGE_SIZE=4096`       - Page size.
* `-DNS_MEMORY_PAGE_LEN=4096`    - Number of pages to fit inside the pager buffer pool.
* `-DNS_WAL_BUFFER_CAP=1048576`  - The size of the internal in memory WAL.

These should rarely be changed:

* `-DTSIZE_BITS=32`              - Number of bits needed to represent the size of a data type.
* `-DPSIZE_BITS=32`              - Number of bits needed to represent the size of an index into a page.
* `-DBSIZE_BITS=64`              - Number of bits needed to represent the size of an index into an array.
* `-DPGNO_BITS=64`               - Number of bits needed to represent the size of a page number.
* `-DTXID_BITS=32`               - Number of bits needed to represent the size of a transaction id.
* `-DLSN_BITS=64`                - Number of bits needed to represent the size of a log sequence number.
* `-DPGH_BITS=8`                 - Number of bits needed to represent the size of a page header.
* `-DWLH_BITS=8`                 - Number of bits needed to represent the size of a log header.

Or you can choose from a preset:
* `--preset release`             - Optimized build with tools and samples.
* `--preset debug`               - Debug build with tests, ASan, and logging.
* `--preset package`             - Portable, stripped build for packaging.
* `--preset ci-tests`            - Debug + ASan + tests. Used by CI.
* `--preset ci-release-tests`    - Release + tests. Used by CI.
* `--preset ci-valgrind`         - Debug + tests, no ASan. Used by CI under Valgrind.
* `--preset ci-coverage`         - Debug + tests + gcov instrumentation. Used by CI.
