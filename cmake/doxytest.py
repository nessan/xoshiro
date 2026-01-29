#!/usr/bin/env python3
"""
Script to extract test/example code from comments in supported source files and generate standalone C++ programs.

Usage: doxytest.py [options] <path_to_directory_or_file> [<path_to_directory_or_file> ...]

Options:
  -d, --dir DIR           Output directory for generated .cpp files (default: current directory).
  -e, --extensions EXT(S) Comma/space/semicolon separated list of file extensions to scan (default: .h, .hpp).
  -i, --include HEADER    A header file to include in generated test files (can be specified multiple times).
  -f, --force             Force regeneration of test files even if they exist and are newer than the header file.
  -s, --silent            Suppress progress messages (error messages are still shown).
  -a, --always            Always generate an trivial test file even if no test code blocks are found.
  -p, --prefix PREFIX     Prefix to use for generated test filenames (default: doxy_).
  -m, --max_fails COUNT   Maximum number of allowed test failures before aborting (default: 10).
  -c, --combined [NAME]   Generate only a combined test file containing all discovered test cases.
                          If NAME is provided it is used as the basename of the generated file.
                          Defaults to `doxytests.cpp` when omitted.

Examples:
  doxytest.py include/project/foo.h
  doxytest.py include/project/ include/more_headers/
  doxytest.py include/project/foo.h --dir tests
  doxytest.py include/project/ -d /tmp/output
  doxytest.py include/project/foo.h -i "<other_project/Bar.h>" -i "<iostream>"
  doxytest.py include/project/foo.h --include "<other_project/Bar.h>" --include "<map>"
  doxytest.py include/project/foo.h --force
  doxytest.py include/project/ --silent
  doxytest.py include/project/foo.h --always
  doxytest.py include/project/ --combined
  doxytest.py include/project/ --combined all_tests_in_one

If given a file `foo.h`, the script creates `doxy_foo.cpp` in the output directory by default (configurable via --prefix).
If given a directory, it scans for supported file extensions (default: .h, .hpp) and creates separate test files for each one.
We always include `<print.h>` at the top of each test file, if you need other headers, you can specify them with the -i option.

Note:
By default, we do not generate a test file if no test code blocks are found but if the --always flag is set, we generate a trivial test file
which prints a message along the lines of "No test code found in foo.h" and exits.

SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
SPDX-License-Identifier: MIT
"""

import sys
import os
import argparse
import re
from datetime import datetime
from pathlib import Path


DEFAULT_SUPPORTED_SUFFIXES = (".h", ".hpp")

HEADER_INCLUDE_SUFFIXES = {
    ".h",
    ".hpp",
    ".hh",
    ".hxx",
    ".hp",
    ".h++",
}

COMMENT_PREFIX_CANDIDATES = (
    "///",
    "//!",
    "//",
    "/*!",
    "/**",
    "/*",
    " *",
    "*",
)


def format_suffix_display(suffixes):
    return ", ".join(suffixes)


def format_suffix_patterns(suffixes):
    return ", ".join(f"*{suffix}" for suffix in suffixes)


def parse_extensions_argument(raw_value):
    if raw_value is None:
        return None
    tokens = re.split(r"[,\s;]+", raw_value.strip())
    normalized = []
    for token in tokens:
        token = token.strip()
        if not token:
            continue
        if not token.startswith("."):
            token = f".{token}"
        token = token.lower()
        if token not in normalized:
            normalized.append(token)
    if not normalized:
        raise ValueError("No valid file extensions were provided for --extensions.")
    return tuple(normalized)


def is_header_like_suffix(suffix):
    if not suffix:
        return False
    return suffix.lower() in HEADER_INCLUDE_SUFFIXES


def _strip_comment_prefix(line, allowed_prefixes=None):
    """
    Remove a recognised documentation comment prefix from the line.

    Parameters:
        line: Original line (including trailing newline).
        allowed_prefixes: Optional iterable restricting which prefixes may be stripped.
            When empty (but not None) we avoid stripping altogether.

    Returns:
        A tuple of (text_without_prefix, text_without_prefix_lstrip, matched_prefix or None).
        The text values do not include the trailing newline.
    """

    stripped_leading = line.lstrip()
    matched_prefix = None
    remainder = stripped_leading

    for candidate in COMMENT_PREFIX_CANDIDATES:
        if stripped_leading.startswith(candidate):
            if allowed_prefixes is not None and candidate not in allowed_prefixes:
                continue
            matched_prefix = candidate
            remainder = stripped_leading[len(candidate) :]
            break

    if matched_prefix is not None:
        if remainder.startswith((" ", "\t")):
            remainder = remainder[1:]
        without_newline = remainder.rstrip("\n")
        return without_newline, without_newline.lstrip(), matched_prefix

    without_newline = line.rstrip("\n")
    return without_newline, without_newline.lstrip(), None


def _allowed_prefixes_for_block(prefix):
    """Return the set of prefixes that should be stripped inside a block."""
    if prefix in {"/*!", "/**", "/*"}:
        return {"*", " *"}
    if prefix in {"*", " *"}:
        return {"*", " *"}
    if prefix in {"///", "//!", "//"}:
        return {prefix}
    if prefix:
        return {prefix}
    return set()


def build_arg_parser():
    parser = argparse.ArgumentParser(
        description="Extracts test code from fenced code blocks in documentation and generates standalone C++ test programs.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  doxytest.py include/project/foo.h
  doxytest.py include/project/
  doxytest.py include/project/ --extensions "h, cpp, md"
  doxytest.py include/project/foo.h --dir tests
  doxytest.py include/project/ -d /tmp/output
  doxytest.py include/project/foo.h -i "<other_project/Bar.h>" -i "<iostream>"
  doxytest.py include/project/foo.h --include "<other_project/Bar.h>" --include "<map>"
  doxytest.py include/project/foo.h --force
  doxytest.py include/project/ --silent
  doxytest.py include/project/ --always
  doxytest.py include/project/ --combined
  doxytest.py include/project/ --prefix test_
  doxytest.py include/project/foo.h --max_fails 3
        """,
    )
    parser.add_argument(
        "input_paths",
        nargs="*",
        help="Files or directories to scan for embedded test code.",
    )
    parser.add_argument(
        "-e",
        "--extensions",
        help=(
            "List of file extensions to scan for tests (default: "
            f"{format_suffix_display(DEFAULT_SUPPORTED_SUFFIXES)}). "
            "Separate values with commas, whitespace, or semicolons."
        ),
    )
    parser.add_argument(
        "-d",
        "--dir",
        dest="output_dir",
        default=".",
        help="Output directory for generated .cpp files (default: current directory).",
    )
    parser.add_argument(
        "-i",
        "--include",
        action="append",
        help="Additional header include to add to generated files (repeatable).",
    )
    parser.add_argument(
        "-f",
        "--force",
        action="store_true",
        help="Force regeneration even if outputs are newer than inputs.",
    )
    parser.add_argument(
        "-s",
        "--silent",
        action="store_true",
        help="Suppress progress messages (errors are still shown).",
    )
    parser.add_argument(
        "-a",
        "--always",
        action="store_true",
        help="Always generate a trivial test file even if no test blocks are found.",
    )
    parser.add_argument(
        "-p",
        "--prefix",
        default="doxy_",
        help="Filename prefix for generated test files (default: doxy_).",
    )
    parser.add_argument(
        "-m",
        "--max_fails",
        type=int,
        default=10,
        help="Maximum number of allowed test failures before aborting (default: 10).",
    )
    parser.add_argument(
        "-c",
        "--combined",
        nargs="?",
        const="doxytests.cpp",
        help="Generate a combined test file (optional name, default doxytests.cpp).",
    )
    return parser


REQUIRED_INCLUDES = [
    "<cstdlib>",
    "<exception>",
    "<format>",
    "<print>",
    "<source_location>",
    "<string>",
    "<string_view>",
    "<tuple>",
    "<type_traits>",
    "<utility>",
    "<vector>",
]


def compute_header_include(header_file_path, output_directory):
    """Return a quoted include string for the header relative to the output directory."""
    header_path = Path(header_file_path).resolve()
    output_dir_path = Path(output_directory).resolve()
    try:
        relative_str = os.path.relpath(header_path, start=output_dir_path)
        relative_path = Path(relative_str)
    except ValueError:
        # Fallback to absolute path when a relative path cannot be constructed (e.g., different drives).
        relative_path = header_path
    include_path = relative_path.as_posix()
    return f'"{include_path}"'


def build_include_block(user_includes, extra_includes=None):
    """Return a list of unique includes (required + user)."""
    include_block = []
    for inc in (extra_includes or []) + REQUIRED_INCLUDES + (user_includes or []):
        if not inc:
            continue
        inc = inc.strip()
        if not inc:
            continue
        if inc.startswith("<") and inc.endswith(">"):
            formatted = inc
        elif inc.startswith('"') and inc.endswith('"'):
            formatted = inc
        else:
            formatted = f'"{inc}"'
        if formatted not in include_block:
            include_block.append(formatted)
    return include_block


def build_helper_block(max_fails):
    """Return the inline helper code used by generated tests."""
    return [
        "",
        "// We use our own assert macros instead of the standard ones.",
        "#ifdef assert",
        "    #undef assert",
        "#endif",
        "#ifdef assert_eq",
        "    #undef assert_eq",
        "#endif",
        "",
        "#define assert(cond, ...) \\",
        "    if(!(cond)) doxy::failed(#cond, header_file, header_line __VA_OPT__(, __VA_ARGS__));",
        "",
        "#define assert_eq(a, b, ...) \\",
        "    if(!((a) == (b))) doxy::failed_eq(#a, #b, (a), (b), header_file, header_line __VA_OPT__(, __VA_ARGS__));",
        "",
        "namespace doxy {",
        "",
        "// Maximum number of allowed failures before we exit the program.",
        f"std::size_t max_fails = {max_fails};",
        "",
        "// A simple exception class for test failures.",
        "struct error : public std::exception {",
        "    explicit error(std::string message) : m_message(std::move(message)) {}",
        "    const char* what() const noexcept override { return m_message.c_str(); }",
        "    std::string m_message;",
        "};",
        "",
        "// Program exit (possibly break in `doxy::exit` if you are debugging a test failure).",
        "void exit(int status) { ::exit(status); }",
        "",
        "// Handle boolean condition assertion evaluation failures.",
        "template <typename... Args>",
        "void",
        "failed(std::string_view cond_str, std::string_view hdr_file, std::size_t hdr_line, ",
        '       std::string_view msg_format = "", Args&&... msg_args) {',
        '    auto what = std::format("\\nFAILED `assert({})` [{}:{}]\\n", cond_str, hdr_file, hdr_line);',
        "    if (!msg_format.empty()) {",
        "        auto arg_storage = std::tuple<std::decay_t<Args>...>(std::forward<Args>(msg_args)...);",
        "        auto format_args = std::apply([](auto&... values) { return std::make_format_args<std::format_context>(values...); }, arg_storage);",
        "        what += std::vformat(msg_format, format_args);",
        "        what.push_back('\\n');",
        "    }",
        "    throw error{std::move(what)};",
        "}",
        "",
        "// Handle equality assertion evaluation failures.",
        "template <typename LHS, typename RHS, typename... Args>",
        "void",
        "failed_eq(std::string_view lhs_str, std::string_view rhs_str, const LHS& lhs, const RHS& rhs,",
        '          std::string_view hdr_file, std::size_t hdr_line, std::string_view msg_format = "", Args&&... msg_args) {',
        '    auto what = std::format("\\nFAILED `assert_eq({}, {})` [{}:{}]\\n", lhs_str, rhs_str, hdr_file, hdr_line);',
        "    if (!msg_format.empty()) {",
        "        auto arg_storage = std::tuple<std::decay_t<Args>...>(std::forward<Args>(msg_args)...);",
        "        auto format_args = std::apply([](auto&... values) { return std::make_format_args<std::format_context>(values...); }, arg_storage);",
        "        what += std::vformat(msg_format, format_args);",
        "        what.push_back('\\n');",
        "    }",
        '    what += std::format("lhs = {}\\n", lhs);',
        '    what += std::format("rhs = {}\\n", rhs);',
        "    throw error{std::move(what)};",
        "}",
        "",
        "} // namespace doxy",
        "",
    ]


def extract_code_blocks(file_path):
    """Extract code blocks from triple backticks in the file."""
    with open(file_path, "r") as f:
        lines = f.readlines()

    code_blocks = []
    i = 0

    while i < len(lines):
        block_line, block_line_stripped, matched_prefix = _strip_comment_prefix(
            lines[i]
        )

        if not block_line_stripped.startswith("```"):
            i += 1
            continue

        language = block_line_stripped[3:].strip().lower()
        if language in ("", "cpp", "c++"):
            block_kind = "test"
        elif language in ("doxy", "doxytest"):
            block_kind = "setup"
        else:
            block_kind = None

        start_line = i + 1  # Line numbers are 1-based
        allowed_prefixes = _allowed_prefixes_for_block(matched_prefix)

        code_content = []
        i += 1  # Move past the opening ```

        # Collect lines until we find the closing ```
        while i < len(lines):
            current_line, current_line_stripped, _ = _strip_comment_prefix(
                lines[i], allowed_prefixes
            )
            if current_line_stripped.startswith("```"):
                break
            if block_kind is not None:
                code_content.append(current_line)
            i += 1

        # Skip the closing ``` if present
        if i < len(lines):
            i += 1

        if block_kind is None:
            continue

        code_text = "\n".join(code_content).strip()
        if code_text:
            code_blocks.append(
                {
                    "line": start_line,
                    "code": code_text,
                    "kind": block_kind,
                }
            )

    return code_blocks


def generate_empty_test_file(header_file_path, header_include):
    """Generate an empty test file when no code blocks are found."""
    header_path = Path(header_file_path).resolve()
    header_display = header_path.name
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    header_comment = f"// Input file(s): `{header_display}`"
    content = []
    content.append(
        "// This file was generated by running the script `doxytest.py` to extract tests from comments in input files."
    )
    content.append(header_comment)
    content.append("// Do not edit this file manually -- it may be overwritten.")
    content.append(f"// Generated on: {timestamp}")
    content.append("")
    if header_include:
        content.append(f"#include {header_include}")
    content.append("#include <print>")
    content.append("")
    content.append("int main() {")
    content.append(
        f'    std::println(stderr, "No test cases were found in `{header_display}`.");'
    )
    content.append("}")
    return "\n".join(content)


def build_test_main_block(
    intro_block,
    failure_summary_line,
    summary_block,
    test_body_lines,
    *,
    additional_variable_lines=None,
):
    """Assemble the shared C++ test harness for generated tests."""
    if additional_variable_lines is None:
        additional_variable_lines = []

    lines = [
        "int",
        "main() {",
    ]
    lines.extend(intro_block)
    lines.append("")
    lines.extend(
        [
            "    // Number of failed doctests (we exit the program if this exceeds doxy::max_fails).",
            "    std::size_t fails = 0;",
            "",
        ]
    )
    lines.extend(
        [
            "    // Variables used to track the current test.",
        ]
    )
    lines.extend(additional_variable_lines)
    lines.extend(
        [
            "    std::size_t header_line = 0;",
            "    std::size_t test = 0;",
            "    auto test_passed = true;",
            "",
            "    // Cache of all failed test messages.",
            "    std::vector<std::string> failed_messages;",
            "",
            "    // Each test failure is handled the same way:",
            "    auto handle_failure = [&](std::string_view message) {",
            "        test_passed = false;",
            "        fails++;",
            '        std::println(stderr, "FAIL");',
            '        std::println(stderr, "{}", message);',
            "        failed_messages.push_back(std::string(message));",
            "        if (fails >= doxy::max_fails) {",
            "            std::println(stderr);",
            f"            {failure_summary_line}",
            '            for (const auto& msg : failed_messages) std::print(stderr, "{}", msg);',
            "            doxy::exit(1);",
            "        };",
            "    };",
        ]
    )
    lines.extend(test_body_lines)
    lines.extend(summary_block)
    lines.append("")
    lines.append("    // Return 1 if there were any failures, 0 otherwise.")
    lines.append("    return fails > 0 ? 1 : 0;")
    lines.append("}")
    return lines


def generate_test_file(
    header_file_path, code_blocks, includes=None, header_include=None, *, max_fails
):
    """Generate the test file content."""
    header_path = Path(header_file_path).resolve()
    header_name = header_path.stem
    header_display = header_path.name
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    header_comment = f"// Input file(s): `{header_display}`"
    include_block = build_include_block(
        includes, extra_includes=[header_include] if header_include else None
    )

    test_blocks = [block for block in code_blocks if block["kind"] == "test"]
    test_count = len(test_blocks)
    test_count_literal = f"{test_count}"

    content = [
        "// This file was generated by running the script `doxytest.py` to extract tests from comments in input files.",
        header_comment,
        "// Do not edit this file manually -- it may be overwritten.",
        f"// Generated on: {timestamp}",
    ]

    if include_block:
        content.append("")
        content.extend(f"#include {include}" for include in include_block)

    content.extend(build_helper_block(max_fails))

    intro_block = [
        "    // Trace the source for the tests.",
        f'    auto header_file = "{header_display}";',
        f"    std::size_t test_count = {test_count_literal};",
        '    std::println(stderr, "Running {} tests extracted from: `{}`", test_count, header_file);',
    ]

    test_body_lines = []
    pending_setups = []
    run_comment_emitted = False
    for block in code_blocks:
        kind = block["kind"]
        code = block["code"]
        line_num = block["line"]

        if kind == "setup":
            pending_setups.append(block)
            continue
        if kind != "test":
            continue

        if pending_setups:
            test_body_lines.append("    // Doxytest Setup Code code ...")
            for setup_block in pending_setups:
                for line in setup_block["code"].split("\n"):
                    test_body_lines.append(f"    {line}" if line.strip() else "")
            test_body_lines.append("")
            pending_setups.clear()

        if not run_comment_emitted:
            test_body_lines.append("    // Run the tests ...")
            run_comment_emitted = True

        test_body_lines.append(f"    header_line = {line_num};")
        test_body_lines.append("    test_passed = true;")
        test_body_lines.append(
            '    std::print(stderr, "test {}/{} ({}:{}) ... ", ++test, test_count, header_file, header_line);'
        )
        test_body_lines.append("    try {")
        for line in code.split("\n"):
            test_body_lines.append(f"        {line}" if line.strip() else "")
        test_body_lines.append(
            "    } catch (const doxy::error& failure) { handle_failure(failure.what()); }"
        )
        test_body_lines.append('    if (test_passed) std::println(stderr, "pass");')
        test_body_lines.append("")

    summary_block = [
        "    // Print a summary of the tests results.",
        "    if (fails == 0) {",
        '        std::println(stderr, "[{}] All {} tests PASSED", header_file, test_count);',
        "    } else {",
        "        std::println(stderr);",
        '        std::println(stderr, "Test FAIL summary for `{}`: Ran {} of a possible {} tests, PASSED: {}, FAILED: {}", header_file, test, test_count, test - fails, fails);',
        '        std::print(stderr, "--------------------------------------------------------------------------------------");',
        '        for (const auto& msg : failed_messages) std::print(stderr, "{}", msg);',
        "    }",
    ]

    failure_summary_line = (
        'std::println(stderr, "Hit the maximum allowed number of failures ({}) for `{}`:", '
        "doxy::max_fails, header_file);\n"
        '            std::println(stderr, "Managed to run {} of a possible {} tests, PASSED: {}, FAILED: {}", '
        "test, test_count, test - fails, fails);\n"
        '            std::println(stderr, "Here is a summary of the failed tests:");'
    )

    content.extend(
        build_test_main_block(
            intro_block,
            failure_summary_line,
            summary_block,
            test_body_lines,
        )
    )

    return "\n".join(content)


def generate_combined_test_file(
    entries, includes=None, combined_output_path=None, *, max_fails
):
    """Generate the combined test file content covering all entries."""
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    resolved_headers = {str(Path(entry["header_file"]).resolve()) for entry in entries}
    header_list = sorted(resolved_headers)
    total_tests = sum(
        sum(1 for block in entry["code_blocks"] if block["kind"] == "test")
        for entry in entries
    )
    header_count = len(resolved_headers)
    combined_output_dir = (
        Path(combined_output_path).resolve().parent
        if combined_output_path is not None
        else Path(".").resolve()
    )
    header_includes = list(
        dict.fromkeys(
            compute_header_include(entry["header_file"], combined_output_dir)
            for entry in entries
            if is_header_like_suffix(Path(entry["header_file"]).suffix)
        )
    )
    include_block = build_include_block(
        includes, extra_includes=header_includes if header_includes else None
    )

    content = [
        "// This combined test file was generated by running the script `doxytest.py` to extract tests from comments in input files."
    ]
    if header_list:
        content.append(
            f'// Input file(s): {", ".join(f"`{Path(name).name}`" for name in header_list)}'
        )
    else:
        content.append("// Input file(s): (none)")
    content.append("// Do not edit this file manually -- it may be overwritten.")
    content.append(f"// Generated on: {timestamp}")

    if include_block:
        content.append("")
        content.extend(f"#include {include}" for include in include_block)

    if total_tests == 0:
        content.extend(build_helper_block(max_fails))
        content.extend(
            [
                "int",
                "main() {",
                '    std::println(stderr, "No tests were discovered in the requested inputs.");',
                "}",
            ]
        )
        return "\n".join(content)

    content.extend(build_helper_block(max_fails))

    intro_block = [
        f"    auto header_count = {header_count};",
        f"    std::size_t test_count = {total_tests};",
        '    std::println(stderr, "Running {} tests extracted from {} input files.", test_count, header_count);',
    ]

    test_body_lines = []
    for entry in entries:
        header_display = entry.get("header_display", Path(entry["header_file"]).name)
        test_body_lines.append(f'    header_file = "{header_display}";')
        pending_setups = []
        run_comment_emitted = False
        for block in entry["code_blocks"]:
            kind = block["kind"]
            code = block["code"]
            line_num = block["line"]

            if kind == "setup":
                pending_setups.append(block)
                continue
            if kind != "test":
                continue

            if pending_setups:
                test_body_lines.append("    // Doxytest Setup Code code ...")
                for setup_block in pending_setups:
                    for line in setup_block["code"].split("\n"):
                        test_body_lines.append(f"    {line}" if line.strip() else "")
                test_body_lines.append("")
                pending_setups.clear()

            if not run_comment_emitted:
                test_body_lines.append("    // Run the tests ...")
                run_comment_emitted = True

            test_body_lines.append(f"    header_line = {line_num};")
            test_body_lines.append("    test_passed = true;")
            test_body_lines.append(
                '    std::print(stderr, "test {}/{} ({}:{}) ... ", ++test, test_count, header_file, header_line);'
            )
            test_body_lines.append("    try {")
            for line in code.split("\n"):
                test_body_lines.append(f"        {line}" if line.strip() else "")
            test_body_lines.append(
                "    } catch (const doxy::error& failure) { handle_failure(failure.what()); }"
            )
            test_body_lines.append('    if (test_passed) std::println(stderr, "pass");')
            test_body_lines.append("")

    summary_block = [
        "    if (fails == 0) {",
        '        std::println(stderr, "Total tests: {}, all PASSED", test_count);',
        "    } else {",
        "        std::println(stderr);",
        '        std::println(stderr, "Total tests: {}, PASSED: {}, FAILED: {}", test_count, test_count - fails, fails);',
        '        for (const auto& msg : failed_messages) std::print(stderr, "{}", msg);',
        "    }",
    ]

    failure_summary_line = (
        'std::println(stderr, "Hit the maximum allowed number of failures ({}) for the combined test file:", '
        "doxy::max_fails);\n"
        '            std::println(stderr, "Managed to run {} of a possible {} tests, PASSED: {}, FAILED: {}", '
        "test, test_count, test - fails, fails);\n"
        '            std::println(stderr, "Here is a summary of the failed tests:");'
    )

    content.extend(
        build_test_main_block(
            intro_block,
            failure_summary_line,
            summary_block,
            test_body_lines,
            additional_variable_lines=["    std::string_view header_file;"],
        )
    )

    return "\n".join(content)


def should_regenerate_test_file(header_file_path, test_file_path, force=False):
    """Check if the test file should be regenerated.

    Returns True if:
    - The test file doesn't exist
    - The header file is newer than the test file
    - Force flag is set

    Returns False if the test file exists and is newer than the header file.
    """
    if force:
        return True

    if not test_file_path.exists():
        return True

    header_mtime = header_file_path.stat().st_mtime
    test_mtime = test_file_path.stat().st_mtime

    return header_mtime > test_mtime


def ensure_output_directory(output_dir, silent=False):
    """Ensure the output directory exists and is writable."""
    output_path = Path(output_dir)

    # Create directory if it doesn't exist
    if not output_path.exists():
        try:
            output_path.mkdir(parents=True, exist_ok=True)
            if not silent:
                print(f"Created output directory: {output_path}")
        except PermissionError:
            print(f"Error: Permission denied when creating directory '{output_path}'")
            return False
        except Exception as e:
            print(f"Error: Failed to create directory '{output_path}': {e}")
            return False

    # Check if directory is writable
    if not os.access(output_path, os.W_OK):
        print(f"Error: Directory '{output_path}' is not writable")
        return False

    return True


def process_header_file(
    header_file_path,
    output_dir,
    includes=None,
    force=False,
    silent=False,
    always=False,
    combined_entries=None,
    file_prefix="doxy_",
    generate_individual=True,
    *,
    max_fails,
    output_basename=None,
):
    """Process a single header file and optionally generate its test file."""
    header_path = Path(header_file_path)
    header_name = header_path.stem
    header_display = header_path.name
    header_suffix = header_path.suffix.lower()
    output_name = output_basename or header_name
    test_file_path = Path(output_dir) / f"{file_prefix}{output_name}.cpp"
    header_include = None
    if is_header_like_suffix(header_suffix):
        header_include = compute_header_include(header_file_path, test_file_path.parent)

    # Extract code blocks
    code_blocks = extract_code_blocks(header_file_path)
    test_blocks = [block for block in code_blocks if block["kind"] == "test"]

    if not test_blocks:
        if not silent:
            print(f"No test cases found in '{header_file_path}'")

        if not generate_individual:
            return always

        if not always:
            return False  # No file is generated if no test cases are found

        # Generate empty test file when --always flag is set
        test_content = generate_empty_test_file(header_file_path, header_include)

        # Determine output file path
        # Check if we need to regenerate the test file
        if not should_regenerate_test_file(
            Path(header_file_path), test_file_path, force
        ):
            if not silent:
                print(
                    f"Test file '{test_file_path}' is up to date, skipping generation."
                )
            return True

        # Write the empty test file
        try:
            with open(test_file_path, "w") as f:
                f.write(test_content)
        except PermissionError:
            print(f"Error: Permission denied when writing to '{test_file_path}'")
            return False
        except Exception as e:
            print(f"Error: Failed to write file '{test_file_path}': {e}")
            return False

        if not silent:
            print(f"Generated empty test file: {test_file_path}")
        return True

    if not generate_individual:
        if combined_entries is not None:
            combined_entries.append(
                {
                    "header_name": header_name,
                    "header_display": header_display,
                    "header_file": str(header_file_path),
                    "code_blocks": code_blocks,
                }
            )
        return True

    # Generate test file content
    test_content = generate_test_file(
        header_file_path,
        code_blocks,
        includes,
        header_include=header_include,
        max_fails=max_fails,
    )

    # Determine output file path
    # Check if we need to regenerate the test file
    if not should_regenerate_test_file(Path(header_file_path), test_file_path, force):
        if not silent:
            print(f"Test file '{test_file_path}' is up to date, skipping generation.")
        if combined_entries is not None:
            combined_entries.append(
                {
                    "header_name": header_name,
                    "header_display": header_display,
                    "header_file": str(header_file_path),
                    "code_blocks": code_blocks,
                }
            )
        return True

    # Write the test file
    try:
        with open(test_file_path, "w") as f:
            f.write(test_content)
    except PermissionError:
        print(f"Error: Permission denied when writing to '{test_file_path}'")
        return False
    except Exception as e:
        print(f"Error: Failed to write file '{test_file_path}': {e}")
        return False

    if not silent:
        print(
            f"Generated test file: {test_file_path} with {len(test_blocks)} test cases."
        )

    if combined_entries is not None:
        combined_entries.append(
            {
                "header_name": header_name,
                "header_display": header_display,
                "header_file": str(header_file_path),
                "code_blocks": code_blocks,
            }
        )
    return True


def main():
    parser = build_arg_parser()
    args = parser.parse_args()

    if args.extensions:
        try:
            supported_suffixes = parse_extensions_argument(args.extensions)
        except ValueError as exc:
            print(f"Error: {exc}")
            sys.exit(1)
    else:
        supported_suffixes = DEFAULT_SUPPORTED_SUFFIXES

    supported_patterns = format_suffix_patterns(supported_suffixes)
    supported_suffix_display = format_suffix_display(supported_suffixes)

    def _looks_like_path(candidate):
        if candidate is None:
            return False
        candidate_lower = candidate.lower()
        if any(candidate_lower.endswith(suffix) for suffix in supported_suffixes):
            return True
        if any(sep in candidate for sep in (os.sep, "/", "\\")):
            return True
        return Path(candidate).exists()

    combined_basename = args.combined
    input_path_strings = list(args.input_paths)

    if combined_basename is not None and combined_basename != "doxytests":
        if _looks_like_path(combined_basename):
            input_path_strings.insert(0, combined_basename)
            combined_basename = "doxytests"

    if not input_path_strings:
        parser.print_help()
        sys.exit(0)

    input_paths = []
    for path_str in input_path_strings:
        path_obj = Path(path_str)
        if not path_obj.exists():
            print(f"Error: Path '{path_str}' does not exist")
            sys.exit(1)
        input_paths.append(path_obj.resolve())

    # Allow explicit file inputs to extend the supported file suffixes without
    # changing the directory globbing behaviour.
    file_suffixes = [
        path_obj.suffix.lower()
        for path_obj in input_paths
        if path_obj.is_file() and path_obj.suffix
    ]
    additional_suffixes = tuple(
        suffix
        for suffix in dict.fromkeys(file_suffixes)  # Preserves first-seen order.
        if suffix not in supported_suffixes
    )

    file_allowed_suffixes = supported_suffixes + tuple(additional_suffixes)
    file_allowed_suffix_display = format_suffix_display(file_allowed_suffixes)

    output_dir = args.output_dir
    includes = args.include
    force = args.force
    silent = args.silent
    always = args.always
    file_prefix = args.prefix
    max_fails = args.max_fails
    combined_requested = combined_basename is not None

    if max_fails is not None and max_fails <= 0:
        print("Error: --max_fails must be a positive integer")
        sys.exit(1)

    # Ensure output directory exists and is writable
    if not ensure_output_directory(output_dir, silent):
        sys.exit(1)

    combined_entries = [] if combined_requested else None
    total_success = 0

    files_to_process = []
    directory_matches = {}
    seen_files = set()

    for input_path in input_paths:
        if input_path.is_file():
            suffix = input_path.suffix.lower()
            if suffix and suffix not in file_allowed_suffixes:
                print(
                    f"Error: '{input_path}' is not a supported file type ({file_allowed_suffix_display})"
                )
                sys.exit(1)
            if input_path not in seen_files:
                files_to_process.append(input_path)
                seen_files.add(input_path)
        elif input_path.is_dir():
            header_files = sorted(
                (
                    header_file.resolve()
                    for suffix in supported_suffixes
                    for header_file in input_path.glob(f"*{suffix}")
                ),
                key=lambda path: path.name.lower(),
            )

            if not header_files:
                if not silent:
                    print(
                        f"No files matching ({supported_patterns}) found in '{input_path}'"
                    )
                directory_matches[input_path] = []
                continue

            unique_files = []
            for header_file in header_files:
                if header_file not in seen_files:
                    unique_files.append(header_file)
                    files_to_process.append(header_file)
                    seen_files.add(header_file)

            if not unique_files:
                if not silent:
                    print(
                        f"No new files matching ({supported_patterns}) found in '{input_path}'"
                    )
            else:
                if not silent:
                    print(
                        f"Found {len(unique_files)} files ({supported_patterns}) in the directory: `{input_path}`"
                    )
            directory_matches[input_path] = unique_files
        else:
            print(f"Error: '{input_path}' is neither a file nor a directory")
            sys.exit(1)

    output_name_map = {}
    if files_to_process:
        stem_groups = {}
        for file_path in files_to_process:
            stem_groups.setdefault(file_path.stem, []).append(file_path)

        primary_per_stem = {}
        for stem, items in stem_groups.items():
            preferred = None
            for candidate in items:
                if candidate.suffix.lower() in DEFAULT_SUPPORTED_SUFFIXES:
                    preferred = candidate
                    break
            if preferred is None:
                preferred = items[0]
            primary_per_stem[stem] = preferred

        used_output_names = set()
        for file_path in files_to_process:
            stem = file_path.stem or ""
            suffix_with_dot = file_path.suffix.lower()
            suffix = suffix_with_dot.lstrip(".")
            primary = primary_per_stem.get(stem)
            if primary is file_path and is_header_like_suffix(suffix_with_dot):
                base_candidate = stem or "doxyfile"
            else:
                if suffix and stem:
                    base_candidate = f"{stem}_{suffix}"
                elif suffix:
                    base_candidate = suffix
                else:
                    base_candidate = stem or "doxyfile"

            if not base_candidate:
                base_candidate = "doxyfile"

            candidate = base_candidate
            suffix_index = 2
            while candidate in used_output_names:
                candidate = f"{base_candidate}_{suffix_index}"
                suffix_index += 1

            used_output_names.add(candidate)
            output_name_map[file_path] = candidate

    files_encountered = 0

    for input_path in input_paths:
        if input_path.is_file():
            output_basename = output_name_map.get(input_path)
            if output_basename is None:
                continue
            files_encountered += 1
            if process_header_file(
                input_path,
                output_dir,
                includes,
                force,
                silent,
                always,
                combined_entries=combined_entries,
                file_prefix=file_prefix,
                generate_individual=not combined_requested,
                max_fails=max_fails,
                output_basename=output_basename,
            ):
                total_success += 1

        elif input_path.is_dir():
            header_files = directory_matches.get(input_path, [])
            if not header_files:
                continue

            dir_success = 0
            for header_file in header_files:
                output_basename = output_name_map.get(header_file)
                if output_basename is None:
                    continue
                if not silent:
                    print(f"Processing: `{header_file.name}`...")
                files_encountered += 1
                if process_header_file(
                    header_file,
                    output_dir,
                    includes,
                    force,
                    silent,
                    always,
                    combined_entries=combined_entries,
                    file_prefix=file_prefix,
                    generate_individual=not combined_requested,
                    max_fails=max_fails,
                    output_basename=output_basename,
                ):
                    dir_success += 1
                    total_success += 1

            if not silent:
                print(
                    f"Summary: There were {dir_success}/{len(header_files)} files with tests."
                )
        else:
            print(f"Error: '{input_path}' is neither a file nor a directory")
            sys.exit(1)

    if files_encountered == 0:
        print(
            f"No files matching ({supported_patterns}) were found in the provided inputs."
        )
        sys.exit(1)

    if total_success == 0:
        sys.exit(1)

    if combined_requested:
        combined_name = combined_basename
        if not combined_name.endswith(".cpp"):
            combined_name = f"{combined_name}.cpp"
        combined_path = Path(output_dir) / combined_name
        combined_content = generate_combined_test_file(
            combined_entries or [],
            includes,
            combined_output_path=combined_path,
            max_fails=max_fails,
        )
        try:
            with open(combined_path, "w") as f:
                f.write(combined_content)
        except PermissionError:
            print(f"Error: Permission denied when writing to '{combined_path}'")
            sys.exit(1)
        except Exception as e:
            print(f"Error: Failed to write file '{combined_path}': {e}")
            sys.exit(1)
        if not silent:
            segment_count = len(combined_entries or [])
            print(
                f"Generated combined test file: {combined_path} with {segment_count} header segment(s)."
            )


if __name__ == "__main__":
    main()
