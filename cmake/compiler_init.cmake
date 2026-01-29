# ---------------------------------------------------------------------------------------------------------------------
# Set some commonly used compiler warning flags etc.
# See: https://nessan.github.io/cmake
#
# SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
# SPDX-License-Identifier: MIT
# ---------------------------------------------------------------------------------------------------------------------

# We always ask CMake to export all its compile commands to a JSON file.
# That is useable by editors such as VSCode to determine header locations etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Get CMake to ask the compiler to add some color to our lives!
set(CMAKE_COLOR_DIAGNOSTICS ON)

# By default, we want the compiler to perform extra checks.
option(WARNINGS_ARE_PICKY "Make the compiler extra picky!" ON)

# By default, we want the compiler to treat any warning as a hard error and stop compilation.
option(WARNINGS_ARE_ERRORS "Make the compiler barf completely if it encounters any warning!" ON)

# The main event ...
function(compiler_init target_name)

    # Force Microsoft Visual Studio Code to use its newer cross-platform compatible preprocessor
    set(MSVC_FLAGS /Zc:preprocessor /utf-8)

    # Add baseline warnings for Microsoft Visual Studio Code
    set(MSVC_FLAGS
          ${MSVC_FLAGS}
          /w14242 # 'identifier': conversion from 'type1' to 'type1', possible loss of data
          /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
          /w14263 # 'function': member function does not override any base class virtual member function
          /w14265 # 'classname': class has virtual functions, but destructor is not virtual
          /w14287 # 'operator': unsigned/negative constant mismatch
          /we4289 # 'variable': loop control variable declared in the for-loop is used outside the for-loop scope
          /w14296 # 'operator': expression is always 'boolean_value'
          /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
          /w14545 # expression before comma evaluates to a function which is missing an argument list
          /w14546 # function call before comma missing argument list
          /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
          /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
          /w14555 # expression has no effect; expected expression with side- effect
          /w14640 # Enable warning on thread un-safe static member initialization
          /w14826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behaviour.
          /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
          /permissive- # standards conformance mode for MSVC compiler.
    )

    # Baseline enabled warnings that are the same for clang and gcc
    set(CLANG_FLAGS
        -Wcast-align
        -Wconversion
        -Wdeprecated
        -Wdouble-promotion
        -Wformat=2
        -Wnon-virtual-dtor
        -Wnull-dereference
        -Wold-style-cast
        -Woverloaded-virtual
        -Wshadow
        -Wsign-conversion
        -Wundef
        -Wno-unused-function # We often have non-inline functions that aren't used in our test/example programs
    )

    # gcc has much the same interface as clang -- we can just add some more baseline items
    set(GCC_FLAGS
        ${CLANG_FLAGS}
        -Wcast-qual
        -Wdisabled-optimization
        -Wduplicated-branches
        -Wduplicated-cond
        -Winvalid-pch
        -Wlogical-op
        -Wmisleading-indentation
        -Wmissing-include-dirs
        -Wno-dangling-else
        -Wno-format-nonliteral
        -Wno-unused-local-typedefs
        -Wpointer-arith
        -Wredundant-decls
        -Wshift-overflow=2
        -Wsized-deallocation
        -Wuseless-cast
    )

    # Some GCC checks have some false positives so we disable them ...
    # GCC at least as of 12.2 has a bug where the -Wrestrict flag detects some false positives in optimized mode.
    set(GCC_FLAGS ${GCC_FLAGS} -Wno-restrict)

    # GCC at least as of 13.1 has a bug where the -Wnull-dereference flag detects some false positives.
    set(GCC_FLAGS ${GCC_FLAGS} -Wno-null-dereference)

    # Up the level of inspection done by the compiler if asked to do so
    if (WARNINGS_ARE_PICKY)
        set(CLANG_FLAGS ${CLANG_FLAGS} -Wall -Wextra)
        set(GCC_FLAGS   ${GCC_FLAGS}   -Wall -Wextra)
        set(MSVC_FLAGS  ${MSVC_FLAGS}  /W4)
    endif()

    # Make the compiler treat warnings as errors (i.e. stop) if asked to do so. Forces developer to fix them.
    if(WARNINGS_ARE_ERRORS)
        set(CLANG_FLAGS ${CLANG_FLAGS} -Werror)
        set(GCC_FLAGS   ${GCC_FLAGS}   -Werror)
        set(MSVC_FLAGS  ${MSVC_FLAGS}  /WX)
    endif()

    # Pick the correct set of flags for the compiler we are using
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(compiler_flags ${MSVC_FLAGS})
    elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(compiler_flags ${CLANG_FLAGS})
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(compiler_flags ${GCC_FLAGS})
    else()
        message(WARNING "No compiler flags were set for unknown '${CMAKE_CXX_COMPILER_ID}' compiler.")
    endif()

    # Are we looking at a library type target?
    set(target_is_a "")
    get_target_property(target_is_a ${target_name} TYPE)

    # Don't generally want any flags to propagate except for header-only targets.
    set(target_type PRIVATE)
    if(${target_is_a} STREQUAL "INTERFACE_LIBRARY")
        set(target_type INTERFACE)
    endif()

    # Debug builds get the DEBUG flag set otherwise we set the NDEBUG flag
    target_compile_definitions(${target_name} ${target_type} $<IF:$<CONFIG:Debug>, DEBUG=1, NDEBUG=1>)

    # Set the other compilation flags as laid out above.
    target_compile_options(${target_name} ${target_type} ${compiler_flags})

endfunction()
