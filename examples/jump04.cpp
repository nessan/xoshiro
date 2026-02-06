// Timing comparison between our jumps and the pre-canned long-jumps in the equivalent "C" version.
// The`xso` version has to actually compute the jump coefficients so will be slower.
//
// This should be run with a reasonable level of compiler optimization to be meaningful.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>
#include <utilities/utilities.h>
#include "vigna.h"

template<typename New, typename Old>
void
compare(New& x, Old& c) {
    // Print the names of the generators we are working on
    std::print("Comparing {} with {}\n", utilities::type(c), x);

    // Some constants etc.
    constexpr std::size_t n_words = New::word_count();
    constexpr std::size_t n_bits = New::bit_count();

    // Make sure we start both generators with identical states ...
    for (auto i = 0uz; i < n_words; ++i) c.s[i] = x[i];

    // stopwatch to time the two versions
    utilities::stopwatch sw;

    // The precomputed long-jumps for the "C" versions is 2^(3*n_bits/4).
    sw.click();
    c.long_jump();
    sw.click();
    auto old_ms = 1000 * sw.lap();

    // Our version of the jump (not using precomputed jump polynomials so should be slower) ...
    sw.click();
    x.jump(3 * n_bits / 4, true);
    sw.click();
    auto new_ms = 1000 * sw.lap();

    // Check the two generators are still the same by looking at the next output of each.
    always_confirm(x() == c.next(), "MISMATCH");

    // All OK so print the timing info -- expect the "C" versions to be faster as they have pre-canned jumps.
    // However, both versions will be very fast and our jumps can be anything not just the pre-canned ones.
    auto ratio = new_ms / old_ms;
    std::print("Times: (new, old) = ({:4.3f}ms, {:4.3f}ms) => ratio = {:3.0Lf}\n\n", new_ms, old_ms, ratio);
}

int
main() {
    utilities::pretty_print_thousands();

    // clang-format off
    // Each tuple entry pairs "our" generator with the equivalent C version which is wrapped in a struct.
    auto generators = std::tuple{
        // std::pair{xso::xoroshiro_2x32_star{},       old::xoroshiro_2x32_star{}},
        // std::pair{xso::xoroshiro_2x32_star_star{},  old::xoroshiro_2x32_star_star{}},
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