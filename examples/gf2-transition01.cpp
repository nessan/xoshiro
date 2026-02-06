// Extract the transition matrices for our generators and check they work as expected.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>
#include <utilities/utilities.h>
#include <gf2/gf2.h>

// Check that T.s gives back the same as step(s) for one of our RNG's. T is the transition matrix
template<typename RNG>
void
run(RNG& rng) {
    // Print the name of the rng state we are working on.
    std::print("{}\n", rng);

    // Get the transition matrix for this RNG as a gf2::BitMatrix
    auto T = xso::transition_matrix<RNG>();

    // Some constants etc.
    using word_type = typename RNG::word_type;
    using array_type = typename RNG::array_type;
    constexpr auto n_words = RNG::word_count();
    constexpr auto n_bits = RNG::bit_count();

    // Copy the current state to a bit-vector.
    gf2::BitVector<word_type> bits{n_bits};
    for (auto i = 0uz; i < n_words; ++i) bits.set_word(i, rng[i]);

    // Advance the current state using the transition matrix approach
    bits = T * bits;

    // Copy the state to an array of words.
    array_type state;
    for (auto i = 0uz; i < n_words; ++i) state[i] = bits.word(i);

    // Step the state in the more traditional manner.
    rng.step();

    // Copy the rng state to an array of words.
    array_type words;
    for (auto i = 0uz; i < n_words; ++i) words[i] = rng[i];

    // Check that the two versions match.
    always_confirm(state == words, "MISMATCH state = {}, words = {}", state, words);

    // All good
    std::println("Transition Matrix and and rng.step() MATCH!\n");
}

int
main() {
    utilities::pretty_print_thousands();

    // clang-format off
    // All our type aliased generators (different types, so stored in a tuple)
    auto generators = std::tuple{
        xso::xoroshiro_2x32_star{},
        xso::xoroshiro_2x32_star_star{},
        xso::xoshiro_4x32_plus{},
        xso::xoshiro_4x32_plus_plus{},
        xso::xoshiro_4x32_star_star{},
        xso::xoroshiro_2x64_plus{},
        xso::xoroshiro_2x64_plus_plus{},
        xso::xoroshiro_2x64_star_star{},
        xso::xoshiro_4x64_plus{},
        xso::xoshiro_4x64_plus_plus{},
        xso::xoshiro_4x64_star_star{},
        xso::xoshiro_8x64_plus{},
        xso::xoshiro_8x64_plus_plus{},
        xso::xoshiro_8x64_star_star{},
        xso::xoroshiro_16x64_star{},
        xso::xoroshiro_16x64_star_star{},
        xso::xoroshiro_16x64_plus_plus{},
    };
    // clang-format on

    std::apply([&](auto&... rng) { (run(rng), ...); }, generators);
    return 0;
}
