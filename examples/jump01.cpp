// Check that our computed jump coefficients match the pre-canned ones in the "C" versions.
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

    // Jump the two versions by 2^(0.50*n_bits) steps.
    c.jump();
    x.jump(n_bits / 2, true);

    // Check the two generators are still the same by looking at the next output of each.
    always_confirm(x() == c.next(), "MISMATCH on jump J = 2^(0.50*n_bits)");

    // Jump the two versions from there by 2^(0.75*n_bits) steps.
    c.long_jump();
    x.jump(3 * n_bits / 4, true);

    // Check the two generators are still the same by looking at the next output of each.
    always_confirm(x() == c.next(), "MISMATCH on jump J = 2^(0.75*n_bits)");

    // All OK
    std::print("jump() & long-jump() MATCH!\n\n");
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