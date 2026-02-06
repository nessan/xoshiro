// Runs a timing comparison between our xoshiro and the equivalent "C" versions from the author's website.
// This needs to be run with a reasonable level of compiler optimization to be meaningful.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>
#include <utilities/utilities.h>
#include "vigna.h"

// Compare one of our xoshiro/xoroshiro classes with the equivalent "C" version in a timing test
// Starting from the same state we run a large number of trials to compare timing between the new and original versions
template<typename New, typename Old>
void
compare(New& x, Old& c) {
    // Print the names of the generators we are working on
    std::print("Comparing {} with {}\n", utilities::type(c), x);

    // Make sure we start both generators with identical states ...
    for (auto i = 0uz; i < x.word_count(); ++i) c.s[i] = x[i];

    // Timing Test: Generate many samples from the two versions of the generator and see how long those runs take
    std::size_t n_trials = 1'000'000'000;

    // Space to store the samples
    std::uint64_t rc, rx;

    // stopwatch to time the two versions
    utilities::stopwatch sw;

    // Time the C version
    sw.click();
    for (auto i = 0uz; i < n_trials; ++i) { rc = c.next(); }
    sw.click();
    auto c_secs = sw.lap();

    // Time the X version
    sw.click();
    for (auto i = 0uz; i < n_trials; ++i) { rx = x(); }
    sw.click();
    auto x_secs = sw.lap();

    // Check to see whether the two versions match.
    always_confirm(rc == rx, "Generator mismatch! rc = {}, rx = {}\n", rc, rx);

    // All OK so print the timing info.
    std::print("{:L} calls took (old, new): {:.2f}s, {:.2f}s\n\n", n_trials, c_secs, x_secs);
}

int
main() {
    utilities::pretty_print_thousands();

    // clang-format off
    // Each tuple entry pairs "our" generator with the equivalent C version which is wrapped in a struct.
    auto generators = std::tuple{
        std::pair{xso::xoroshiro_2x32_star{},       old::xoroshiro_2x32_star{}},
        std::pair{xso::xoroshiro_2x32_star_star{},  old::xoroshiro_2x32_star_star{}},
        std::pair{xso::xoshiro_4x32_plus{},         old::xoshiro_4x32_plus{}},
        std::pair{xso::xoshiro_4x32_plus_plus{},    old::xoshiro_4x32_plus_plus{}},
        std::pair{xso::xoshiro_4x32_star_star{},    old::xoshiro_4x32_star_star{}},
        std::pair{xso::xoroshiro_2x64_plus{},       old::xoroshiro_2x64_plus{}},
        std::pair{xso::xoroshiro_2x64_plus_plus{},  old::xoroshiro_2x64_plus_plus{}},
        std::pair{xso::xoroshiro_2x64_star_star{},  old::xoroshiro_2x64_star_star{}},
        std::pair{xso::xoshiro_4x64_plus{},         old::xoshiro_4x64_plus{}},
        std::pair{xso::xoshiro_4x64_plus_plus{},    old::xoshiro_4x64_plus_plus{}},
        std::pair{xso::xoshiro_4x64_star_star{},    old::xoshiro_4x64_star_star{}},
        std::pair{xso::xoshiro_8x64_plus{},         old::xoshiro_8x64_plus{}},
        std::pair{xso::xoshiro_8x64_plus_plus{},    old::xoshiro_8x64_plus_plus{}},
        std::pair{xso::xoshiro_8x64_star_star{},    old::xoshiro_8x64_star_star{}},
        std::pair{xso::xoroshiro_16x64_star{},      old::xoroshiro_16x64_star{}},
        std::pair{xso::xoroshiro_16x64_star_star{}, old::xoroshiro_16x64_star_star{}},
        std::pair{xso::xoroshiro_16x64_plus_plus{}, old::xoroshiro_16x64_plus_plus{}},
    };
    // clang-format on

    // Run the comparisons ...
    std::apply([&](auto&... p) { (compare(p.first, p.second), ...); }, generators);

    return 0;
}