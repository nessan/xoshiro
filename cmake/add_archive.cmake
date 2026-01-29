# ---------------------------------------------------------------------------------------------------------------------
# @brief Package a set of assets into an archive.
# @link  https://nessan.github.io/cmake/
#
# SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nessan.fitzmaurice@me.com>
# SPDX-License-Identifier: MIT
# ---------------------------------------------------------------------------------------------------------------------

function(add_archive)

    # Valid list of formats for the fairly limited `CMake -E tar --format` command.
    # Not documented anywhere I can see -- found these  by looking at CMake source code)
    set(valid_formats zip 7zip gnutar pax paxr)

    # Parse all the passed arguments looking for some  keywords & their associated values
    set(boolean_args VERBOSE)
    set(one_value_args ARCHIVE_BASENAME ARCHIVE_FORMAT ARCHIVE_LOCATION ARCHIVE_TARGET)
    set(multi_value_args)
    cmake_parse_arguments(ARG "${boolean_args}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    # Simplify the names of the captured values
    set(verbose  ${ARG_VERBOSE})
    set(basename ${ARG_ARCHIVE_BASENAME})
    set(format   ${ARG_ARCHIVE_FORMAT})
    set(location ${ARG_ARCHIVE_LOCATION})
    set(target   ${ARG_ARCHIVE_TARGET})

    # Anything that wasn't parsed is assumed to be an asset to archive
    set(assets ${ARG_UNPARSED_ARGUMENTS})

    # There should be some assets to archive otherwise we issue a warning and return early
    if(NOT DEFINED assets)
        message(WARNING "No assets to archive so no archive target added to the build!")
        return()
    endif()

    # The assets should all be files or directories
    foreach(asset ${assets})
        if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${asset})
            message(FATAL " Asked to archive '${asset}' but not such file or directory exists!")
        endif()
    endforeach()

    # If the archive basename wasn't specified we default to the project name
    if(NOT DEFINED basename)
        set(basename ${PROJECT_NAME})
    endif()

    # If the archive format wasn't specified we default to zip
    if(NOT DEFINED format)
        set(format zip)
    endif()

    # Make sure the archive format is recognized by the limited `CMake -E tar` command
    if(NOT ${format} IN_LIST valid_formats)
        message(WARNING "CMake tar will not recognize the format '${format} -- using 'zip' instead!")
        set(format zip)
    endif()

    # If the archive location wasn't specified we default to ${CMAKE_CURRENT_BINARY_DIR}
    if(NOT DEFINED location)
        set(location "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    # Full location and name of the archive will be
    set(archive ${location}/${basename}.${format})

    # If the name of the cmake target to create the archive wasn't specified we just use "archive"
    if(NOT DEFINED target)
        set(target "archive")
    endif()

    # On request we will emit a summary of what we are doing
    if(verbose)
        message("${CMAKE_CURRENT_FUNCTION}::")
        message("  Target  - ${target}")
        message("  Archive - ${archive}")
        message("  Assets  - ${assets}")
    endif()

    # Check that the target name is available for use. Fatal error if it isn't
    if(TARGET ${target})
        message(FATAL
                "The cmake target '${target}' is already in use! "
                "Try setting an alternative name by using the 'ARCHIVE_TARGET' keyword.")
    endif()

    # Add the command to create the archive and target to trigger its creation
    add_custom_command(
        OUTPUT ${archive}
        COMMAND ${CMAKE_COMMAND} -E tar c ${archive} --format=${format} -- ${assets}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${assets})
    add_custom_target(${target} DEPENDS ${archive})

endfunction()