# ---------------------------------------------------------------------------------------------------------------------
# @brief Add/remove the SYSTEM attribute to/from target include directories.
# @link  https://nessan.github.io/cmake/
#
# SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nessan.fitzmaurice@me.com>
# SPDX-License-Identifier: MIT
# ---------------------------------------------------------------------------------------------------------------------

# Add the SYSTEM attribute to the include directories for a target
function(systematize target)

    # We use lots of compiler error checking and treat warnings as errors.
    # That is great for our own code but a pain if we are using some external library that generates a lot of warnings
    # from its header files. Fixing them is not generally practical -- instead we get CMake to treat those headers as
    # "system" headers. The compiler ignores warnings from those.
    get_target_property(inc_dir ${target} INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(${target} SYSTEM INTERFACE ${inc_dir})

endfunction()
