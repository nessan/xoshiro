# README

The project uses [Doxytest][], a tool for generating C++ test programs from sample code embedded in header file comments.

This directory contains the generated test files, which are created by running the Python script `cmake/doxytest.py` on the project's header files. That script is automatically invoked by a `CMake` script `doxytest.cmake` whenever the header files are updated.

The overall effect is similar to the builtin documentation test facility in Rust.

If we have a header file `Foo.h` then the generated test file will be named `test_Foo.cpp`.
In `Foo.h`, we might have a header comment for its `bar()` method looking something like:

````c++
/// @brief A brief description of the method in question.
///
/// Perhaps more detailed notes, caveats, etc. followed by
///
/// # Example (written in the form of tests in a code block)
/// ```
/// Foo foo;
/// assert_equal(foo.bar(), 42);
/// ```
````

The Python script extracts all the examples from the `Foo.h`, putting each one in its own block with some numbering information.
The generated test file will be called `test_Foo.cpp`.

When you build and run `test_Foo` all the test blocks are executed. The  `doxytest.cmake` script ensures that all the generated test files are built and run as part of the standard build and test process for the project.

If any test fails you get information about the failure and also can see the line in the header file where the particular example block came from.

_Note: There is no point editing the generated test files by hand as they will be overwritten by the script on the next run._

See The [Doxytest][] documentation for more information about how to use Doxytest in your own projects.

<!-- Reference Links -->

[Doxytest]: https://nessan.github.io/doxytest/
