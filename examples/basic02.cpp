// Runs a basic comparison between our xoshiro and the "C" versions more or less from the author's website.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>
#include <utilities/utilities.h>
#include "vigna.h"

/// Compare one of our xoshiro/xoroshiro classes with its equivalent "C" version,
/// Starting from the same state we run through a small number of generation cycles checking the outputs match.
template<typename New, typename Old>
void
compare(New& x, Old& c) {
    // Print the names of the generators we are working on
    std::print("Comparing {} with {}\n", utilities::type(c), x);

    // Make sure we start both generators with identical states ...
    for (auto i = 0uz; i < x.word_count(); ++i) c.s[i] = x[i];

    // Basic Test: Run the two versions of the generator for a number of trials and check the outputs match
    std::size_t n_trials = 10;
    for (size_t i = 0; i < n_trials; ++i) always_confirm(x() == c.next(), "MISMATCH on trial {}", i);

    // All OK.
    std::print("Success - all {} trials MATCHED!\n\n", n_trials);
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
