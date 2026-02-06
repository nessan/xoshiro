cmake_minimum_required(VERSION 3.28)

# Our C++ project ...
project(xoshiro DESCRIPTION "xoshiro/xoroshiro random number generators in C++" LANGUAGES CXX)

# Add a target for the "library" we are building -- it is header only and hence INTERFACE.
# Also add an alias that prepends a "namespace". Clients using the alias get better error messages.
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# We use C++20 features.
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

# Where to find the headers.
target_sources(${PROJECT_NAME} INTERFACE
    FILE_SET    library_headers
    TYPE        HEADERS
    BASE_DIRS   include/)

# This library can do more if we allow a dependency on `gf2`-- a C++ library for working in GF(2).
# See the documentation at https://nessan.github.io/gf2.
option(GF2 "Download the `gf2` library for extra functionality?" OFF)
if (GF2)

    # Fetch a minimal archive of the `gf2` library from a remote repo.
    include(FetchContent) 
    FetchContent_Declare(gf2 URL https://github.com/nessan/gf2/releases/download/current/gf2.zip)
    FetchContent_MakeAvailable(gf2)

    # Set a flag to let our library know we have access to the `gf2` library.
    target_compile_definitions(${PROJECT_NAME} INTERFACE GF2)

    # Add the `gf2` dependency and make it transitive so anyone downstream also adds the dependency.
    target_link_libraries(${PROJECT_NAME} INTERFACE gf2::gf2)

endif()

# That's it unless we are developing the library instead of just using it.
if (PROJECT_IS_TOP_LEVEL)

    # Append our local directory of CMake modules to the default ones searched by CMake.
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

    # Prevent in-source builds for the example programs.
    include(disable_in_source_builds)

    # Make the compiler issue warnings for "bad" code etc.
    include(compiler_init)
    compiler_init(${PROJECT_NAME})

    # For neatness, we put the example executables in build/bin/.
    include(GNUInstallDirs)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")

    # Our example programs use the header-only `utilities` library.
    include(fetch_content)
    fetch_content(utilities URL https://github.com/nessan/utilities/releases/download/current/utilities.zip)

    # Our example programs use the header-only `gf2` library.
    fetch_content(gf2 URL https://github.com/nessan/gf2/releases/download/current/gf2.zip)

    # Set a flag to let `xoshiro.h` know we have access to the `gf2` library.
    target_compile_definitions(${PROJECT_NAME} INTERFACE GF2)

    # Walk through the examples/ directory and build a target for each .cpp file with appropriate linkage.
    # We have a CMake module that makes that traversal straightforward.
    include(add_executables)
    add_executables(${CMAKE_SOURCE_DIR}/examples
        COMBINED_TARGET "all_examples"
        LIBRARIES ${PROJECT_NAME}::${PROJECT_NAME} utilities::utilities gf2::gf2)

    # Extract the tests from the comments in the header files and create a target for each corresponding test program.
    # We have a CMake module and a Python script that make that straightforward.
    include(doxytest)
    doxytest(${CMAKE_SOURCE_DIR}/include
        INCLUDES "<xoshiro.h>" LIBRARIES ${PROJECT_NAME}::${PROJECT_NAME} gf2::gf2)

endif()
