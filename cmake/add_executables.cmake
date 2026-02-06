# ---------------------------------------------------------------------------------------------------------------------
# Adds targets for lots of small executables (typically all the files from some directory).
# See: https://nessan.github.io/cmake/
#
# SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nessan.fitzmaurice@me.com>
# SPDX-License-Identifier: MIT
# ---------------------------------------------------------------------------------------------------------------------

# add_executables(PATH [PATH ...]
#                 [LIBRARIES <lib1> <lib2> ...]
#                 [COMBINED_TARGET [<target_name>]])
#
# This function processes source files in the provided PATH arguments (each of which may be a directory or an individual
# source file) and generates corresponding executable targets. When COMBINED_TARGET is provided, it also creates a single
# aggregate target that depends on all created executables, so you can build them in one go.
#
# Each source file is assumed to generate a standalone executable program (so Foo.cpp -> Foo).
#
# Parameters:
#   PATH            - One or more directories and/or individual source files to process.
#   LIBRARIES       - Optional list of libraries to link executables to.
#   COMBINED_TARGET - Optional aggregate target. If present without a name, defaults to "all_examples".
#
# Example:
#   add_executables(examples extra/example.cpp
#       COMBINED_TARGET
#       LIBRARIES gf2::gf2 utilities::utilities)
function(add_executables)

    # First we parse arguments and look for the optional COMBINED_TARGET flag.
    # If present, see if a name follows it,otherwise default to "all_examples".
    set(filtered_args "")
    set(combined_target_requested FALSE)
    set(combined_target_name "")
    set(arg_list ${ARGN})
    list(LENGTH arg_list arg_list_len)
    set(arg_index 0)
    while(arg_index LESS arg_list_len)
        list(GET arg_list ${arg_index} current_arg)
        if(current_arg STREQUAL "COMBINED_TARGET")
            set(combined_target_requested TRUE)
            math(EXPR next_index "${arg_index} + 1")
            if(next_index LESS arg_list_len)
                list(GET arg_list ${next_index} next_arg)
                if(NOT next_arg STREQUAL "LIBRARIES" AND NOT next_arg STREQUAL "COMBINED_TARGET")
                    if(IS_ABSOLUTE "${next_arg}")
                        set(candidate_path "${next_arg}")
                    else()
                        get_filename_component(candidate_path "${next_arg}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
                    endif()
                    if(NOT EXISTS "${candidate_path}" AND NOT IS_DIRECTORY "${candidate_path}")
                        set(combined_target_name "${next_arg}")
                        set(arg_index ${next_index})
                    endif()
                endif()
            endif()
        else()
            list(APPEND filtered_args "${current_arg}")
        endif()
        math(EXPR arg_index "${arg_index} + 1")
    endwhile()

    # Parse the remaining arguments using cmake_parse_arguments.
    set(options)
    set(oneValueArgs)
    set(multiValueArgs LIBRARIES)
    cmake_parse_arguments(NEEDED "${options}" "${oneValueArgs}" "${multiValueArgs}" ${filtered_args})

    # Error out if no source inputs were provided.
    set(source_inputs ${NEEDED_UNPARSED_ARGUMENTS})
    if(NOT source_inputs)
        message(FATAL_ERROR "add_executables: At least one directory or source file must be specified.")
    endif()

    # Collect source files from the provided paths.
    set(valid_extensions ".c" ".cc" ".cxx" ".cpp" ".C" ".c++")
    set(collected_sources "")

    foreach(input_path ${source_inputs})
        if(IS_ABSOLUTE "${input_path}")
            set(abs_input "${input_path}")
        else()
            get_filename_component(abs_input "${input_path}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        endif()

        if(IS_DIRECTORY "${abs_input}")
            file(GLOB_RECURSE dir_sources CONFIGURE_DEPENDS LIST_DIRECTORIES false
                "${abs_input}/*.c"
                "${abs_input}/*.cc"
                "${abs_input}/*.cxx"
                "${abs_input}/*.cpp"
                "${abs_input}/*.C"
                "${abs_input}/*.c++")
            if(dir_sources)
                list(APPEND collected_sources ${dir_sources})
            else()
                message(WARNING "add_executables: No source files found in directory '${abs_input}'.")
            endif()
        elseif(EXISTS "${abs_input}")
            cmake_path(GET abs_input EXT input_ext)
            if(NOT input_ext IN_LIST valid_extensions)
                message(FATAL_ERROR "add_executables: File '${input_path}' does not have a supported C/C++ extension.")
            endif()
            list(APPEND collected_sources "${abs_input}")
        else()
            message(FATAL_ERROR "add_executables: Path '${input_path}' does not exist.")
        endif()
    endforeach()

    # Error out if no source files were collected.
    if(NOT collected_sources)
        message(WARNING "add_executables: No source files were discovered in the provided inputs.")
        return()
    endif()

    # Remove duplicates and sort the collected sources for consistent ordering.
    list(REMOVE_DUPLICATES collected_sources)
    list(SORT collected_sources)

    # Create an executable target for each collected source file.
    set(executable_targets "")
    foreach(src_file ${collected_sources})
        cmake_path(GET src_file STEM target_name)

        if(TARGET ${target_name})
            message(WARNING "add_executables: Target '${target_name}' already exists; skipping source '${src_file}'.")
            get_target_property(existing_target_type ${target_name} TYPE)
            if(existing_target_type STREQUAL "EXECUTABLE")
                list(APPEND executable_targets ${target_name})
            endif()
            continue()
        endif()

        add_executable(${target_name} ${src_file})
        if(NEEDED_LIBRARIES)
            target_link_libraries(${target_name} ${NEEDED_LIBRARIES})
        endif()
        list(APPEND executable_targets ${target_name})
    endforeach()

    # If requested, create the combined aggregate target and set it to depend on all created executables.
    if(combined_target_requested AND executable_targets)
        if(combined_target_name STREQUAL "")
            set(combined_target_name "all_examples")
        endif()
        if(TARGET ${combined_target_name})
            message(WARNING "add_executables: Aggregate target '${combined_target_name}' already exists; skipping.")
        else()
            add_custom_target(${combined_target_name})
            add_dependencies(${combined_target_name} ${executable_targets})
        endif()
    endif()

endfunction()
