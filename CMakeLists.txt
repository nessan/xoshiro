cmake_minimum_required(VERSION 3.24)

# Our C++ project ...
project(xoshiro DESCRIPTION "xoshiro/xoroshiro random number generators in C++" LANGUAGES CXX)

# Add a target for the "library" we are building (it is header only -- hence INTERFACE).
# Also add an alias that prepends a "namespace" -- if clients use that to link to, they get better error messages.
add_library(xoshiro INTERFACE)
add_library(xoshiro::xoshiro ALIAS xoshiro)

# We use some C++20 features.
target_compile_features(xoshiro INTERFACE cxx_std_20)

# Where to find the xoshiro headers (e.g. how to resolve `#include "xoshiro.h"`).
target_include_directories(xoshiro INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include/xoshiro>)

# This library can do more if we allow a dependency on `bit`-- a C++ library for working in GF(2).
# See the documentation at https://nessan.github.io/bit.
option(BIT "Use the `bit` library for extra functionality?" OFF)
if (BIT)

    # Fetch a minimal archive of the `bit` library from a remote repo.
    include(FetchContent)
    FetchContent_Declare(bit URL https://github.com/nessan/bit/releases/download/current/bit.zip)
    FetchContent_MakeAvailable(bit)

    # Set a flag to let our library know we have access to the `bit` library.
    target_compile_definitions(xoshiro INTERFACE BIT)

    # Add the `bit` dependency and make it transitive so anyone downstream also adds the dependency.
    target_link_libraries(xoshiro INTERFACE bit::bit)

endif()

# That's it unless we are developing the library instead of just using it.
if (PROJECT_IS_TOP_LEVEL)

    # Append our local directory of CMake modules to the default ones searched by CMake.
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

    # Prevent in-source builds for the example programs.
    include(disable_in_source_builds)
    disable_in_source_builds()

    # Make the compiler issue warnings for "bad" code etc.
    include(compiler_init)
    compiler_init(xoshiro)

    # For neatness, we put the example executables in build/bin/.
    include(GNUInstallDirs)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")

    # Our example programs use the header-only `utilities` library.
    include(FetchContent)
    FetchContent_Declare(utilities URL https://github.com/nessan/utilities/releases/download/current/utilities.zip)
    FetchContent_MakeAvailable(utilities)

    # Our example programs use the header-only `bit` library.
    FetchContent_Declare(bit URL https://github.com/nessan/bit/releases/download/current/bit.zip)
    FetchContent_MakeAvailable(bit)

    # Set a flag to let `xoshiro.h` know we have access to the `bit` library.
    target_compile_definitions(xoshiro INTERFACE BIT)

    # Walk through the examples/ directory and build a target for each .cpp file with appropriate linkage.
    # We have a CMake module that makes that traversal straightforward.
    include(add_executables)
    add_executables(examples xoshiro::xoshiro bit::bit utilities::utilities)

endif()
