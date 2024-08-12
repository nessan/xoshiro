/// @brief Extract the transition matrices for our generators and check they work as expected.
/// @note  Uses the `bit` library.
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"
#include <bit/bit.h>

/// @brief Check that T.s gives back the same as step(s) for one of our State's. T is the transition matrix
template<typename State>
void
run_check(State& engine)
{
    // Print the name of the engine we are working on.
    std::print("{}\n", engine);

    // Get the transition matrix for this State as a bit::matrix
    auto T = xso::transition_matrix(engine);

    // Some constants etc.
    using word_type = typename State::word_type;
    constexpr std::size_t n_words = State::word_count();
    constexpr std::size_t n_bits = State::bit_count();;

    // Storage where we can go back and forth between bit-space and word-space.
    std::array<word_type, n_words> state;
    bit::vector                    bits{n_bits};

    // Copy the current state to a bit-vector.
    for (std::size_t i = 0; i < n_words; ++i) state[i] = engine[i];
    bits.import_bits(state);

    // Step the state using the transition matrix approach & convert the bits to words.
    bits = bit::dot(T, bits);
    bits.export_bits(state);

    // Step the state in the more traditional manner.
    engine.step();

    // Grab the new engine state in words
    std::array<word_type, n_words> words;
    for (std::size_t i = 0; i < n_words; ++i) words[i] = engine[i];

    // Check to see whether the two versions match.
    verify(state == words, "MISMATCH state = {}, words = {}", state, words);

    // All good
    std::print("Transition Matrix and and engine.step() MATCH!\n\n");
}

int
main()
{
    // Our generators
    xso::xoroshiro_2x32_star       x01;
    xso::xoroshiro_2x32_star_star  x02;
    xso::xoshiro_4x32_plus         x03;
    xso::xoshiro_4x32_plus_plus    x04;
    xso::xoshiro_4x32_star_star    x05;
    xso::xoroshiro_2x64_plus       x06;
    xso::xoroshiro_2x64_plus_plus  x07;
    xso::xoroshiro_2x64_star_star  x08;
    xso::xoshiro_4x64_plus         x09;
    xso::xoshiro_4x64_plus_plus    x10;
    xso::xoshiro_4x64_star_star    x11;
    xso::xoshiro_8x64_plus         x12;
    xso::xoshiro_8x64_plus_plus    x13;
    xso::xoshiro_8x64_star_star    x14;
    xso::xoroshiro_16x64_star      x15;
    xso::xoroshiro_16x64_star_star x16;
    xso::xoroshiro_16x64_plus_plus x17;

    // Print large numbers with commas
    utilities::pretty_print_thousands();

    run_check(x01);
    run_check(x02);
    run_check(x03);
    run_check(x04);
    run_check(x05);
    run_check(x06);
    run_check(x07);
    run_check(x08);
    run_check(x09);
    run_check(x10);
    run_check(x11);
    run_check(x12);
    run_check(x13);
    run_check(x14);
    run_check(x15);
    run_check(x16);
    run_check(x17);

    return 0;
}
