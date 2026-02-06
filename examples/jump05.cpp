// Compare jumping one of our xoshiro/xoroshiro classes with the equivalent "C" version.
// The "C" versions use precomputed jump polynomials and in this test we factor that into the timings.
//
// This needs to be run with a reasonable level of compiler optimization to be meaningful.
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

    // The precomputed jump() for the "C" versions is 2^(n_bits/2).
    // Let's precompute the same thing for our version and give separate times for that piece of the puzzle.
    sw.click();
    auto jump_poly = xso::jump_coefficients<typename New::state_type>(n_bits / 2, true);
    sw.click();
    auto poly_secs = sw.lap();
    std::print("Time to compute jump coefficients: {:4.1Lf}ms\n", 1000 * poly_secs);

    // Time the "C" version for lots of precomputed jumps
    std::size_t n_jumps = 1'000'000;
    sw.click();
    for (std::size_t n = 0; n < n_jumps; ++n) c.jump();
    sw.click();
    auto old_secs = sw.lap();

    // Time our version for the same number of jumps each using the precomputed jump polynomial,
    sw.click();
    for (std::size_t n = 0; n < n_jumps; ++n) x.jump(jump_poly);
    sw.click();
    auto new_secs = sw.lap();

    // Check the two generators are still the same by looking at the next output of each.
    always_confirm(x() == c.next(), "MISMATCH");

    // All OK so print the timing info on just the jump part of the puzzle which should be very similar ...
    auto ratio = new_secs / old_secs;
    std::print("2^{:L} jumps: (new, old) = ({:2.1f}ms, {:2.1f}ms) => ratio = {:3.1Lf}\n\n", n_jumps, 1000 * new_secs,
               1000 * old_secs, ratio);
}

int
main() {
    // Make those large generated random numbers at least somewhat readable.
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
