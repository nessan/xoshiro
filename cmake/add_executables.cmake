# -----------------------------------------------------------------------------
# @brief Add targets for lots of small executables.
# @link  https://nessan.github.io/cmake/
#
# SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nessan.fitzmaurice@me.com>
# SPDX-License-Identifier: MIT
# -----------------------------------------------------------------------------

# group_target(Foo03 target) will return Foo in the `target` argument.
# This is a local helper function used by the main event below.
function(group_target target group_target)

    # NOTE: CMake's regex abiliities are fairly basic but this seems to work OK
    string(REGEX REPLACE "(^[a-zA-Z]*)_*[0-9]+$" "\\1" group ${target})
    set(${group_target} ${group} PARENT_SCOPE)

endfunction()

# check_target("Foo" target) will return a valid target-name in the second
# argument -- typically just Foo. However, if it happens that 'Foo' is already a
# pre-existing target then you will get back something like Foo1, Foo2 etc.
#
# Note this function uses the CMake in/out idiom so the first variable is passed
# as a value (e.g. "Foo" or ${target}) while the second is just a variable name
# which may or may not exist at the time of the call. Call might look like
# `check_target("Foo" target)` or `check_target("$tgt" target)`
#
# This is a local helper function used by the main event below but might be
# useful in other contexts.
function(check_target in out)

    # Start by assuming that the given target is not conflicted
    set(trial ${in})

    # Counter that we will use to create new potential target names if there is a conflict
    set(n_min "2")
    set(n_max "9")
    set(n ${n_min})

    # As long as there is a conflict append -n to the target without going crazy with the size of n ....
    while(TARGET ${trial})
        set(trial ${in}-${n})
        math(EXPR n "${n} + 1")
        if(n GREATER n_max)
            message(FATAL "Cannot create an unconflicted target name for ${in}")
        endif()
    endwhile()

    # Set the unconflicted target name
    set(${out} ${trial} PARENT_SCOPE)

endfunction()


# add_executables(examples, ...) is the main event as descibed in the docs.
function(add_executables dir)

    # Trivial check ...
    if(NOT IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
        message(WARNING "Argument '${dir}' isn't a directory! (Spelling error?)")
        return()
    endif()

    # Grab all the source files from the directory in question (only handle C++/C as yet)
    file(GLOB sources LIST_DIRECTORIES false CONFIGURE_DEPENDS ${dir}/*.cpp ${dir}/*.cc ${dir}/*.c)
    set(all_targets "")

    # Add a target for each file
    foreach(source_file ${sources})

        # File "Foo03.cpp" is expected to yield target "Foo03" in the target variable
        cmake_path(GET source_file STEM target)

        # Might have a target naming conflict (test program Foo linking to library Foo)
        check_target(${target} clean_target)

        # We link any libraries passed at the end of the function invocation.
        add_executable(${clean_target} EXCLUDE_FROM_ALL ${source_file})
        target_link_libraries(${clean_target} ${ARGN})

        # Add the new target to the list of all the targets
        list(APPEND all_targets ${clean_target})

    endforeach()

    # Assuming there are targets to process we now create a target "dir" that expands to all the targets
    # and also group targets so that e.g. Bar will expand to Bar01 Bar02 and Foo to Foo01 Foo2 Foo03 etc.
    list(LENGTH all_targets all_targets_len)
    if(${all_targets_len} GREATER 0)

        # Want to have everything in nice order e.g. Bar0, Bar01, Bar03, Foo01, Foo02, Starter, ...
        list(SORT all_targets)

        # We will keep track of a group with a name like "Bar" and members "Bar01,Bar02,..."
        set(group_target "")     # Initial value
        set(group_list "")       # Initial value

        # Go through the targets and extract the groups and their members
        foreach(exec ${all_targets})

            # Given a name like Foo01 or Foo2 or Foo_12 the following regex will extract the string Foo
            group_target(${exec} exec_group)

            # OK we got our "Foo". Do we need to close out the current group and start a new one?
            if(NOT ${exec_group} STREQUAL group_target)

                # How many targets are in the current group (say it's called Bar)?
                list(LENGTH group_list group_len)

                # Create a target for Bar if that is worth our while (i.e. more than one sub-target)
                if(${group_len} GREATER 1)
                    check_target(${group_target} clean_group_target)
                    add_custom_target(${clean_group_target} DEPENDS ${group_list})
                endif()

                # Start the new group with no members in its corresponding list as yet
                set(group_target ${exec_group})
                set(group_list "")

            endif()

            # Have an appropriate group to add this particular target to
            list(APPEND group_list ${exec})

        endforeach()

        # Finally we create an overall cumulative target 'dir' that expands to Foo01, Foo02, Bar01, Bar02, Bar03 etc.
        check_target(${dir} clean_dir_target)
	    add_custom_target(${clean_dir_target} DEPENDS ${all_targets})

    endif()

endfunction()
