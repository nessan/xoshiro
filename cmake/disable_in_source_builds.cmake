# -----------------------------------------------------------------------------
# @brief Dsable in-source builds. A classic!
# @link  https://nessan.github.io/cmake/
#
# SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nessan.fitzmaurice@me.com>
# SPDX-License-Identifier: MIT
# -----------------------------------------------------------------------------
function(disable_in_source_builds)

    if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
        message(FATAL_ERROR
        "You are trying to compile things in the same location that the source files live in.
        This is considered 'unhygienic' so is not allowed by this project: ${PROJECT_NAME}!
        Please create a subfolder (e.g. `mkdir build; cd build`) and use `cmake ..` from there.

        NOTE: cmake will now (unfortunately) create the file CMakeCache.txt and the directory CMakeFiles/
        in the ${PROJECT_SOURCE_DIR}. So as well as creating the build directory etc. you must delete both
        of those or cmake will refuse to work.")
    endif()

endfunction()

# Immediately run the function -- hust including this file should run it.
disable_in_source_builds()