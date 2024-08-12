/// @brief Just how slow is it to generate random numbers from a transition matrix?
/// @note  Uses the `bit` library.
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"
#include <bit/bit.h>

/// @brief Time how long the transition matrix approach takes versus the conventional step() method.
template<typename State>
void
run(State& engine, std::size_t n_trials)
{
    // Print the name of the generator we are working on
    std::print("{}\n", engine);

    // Get the transition matrix for this State as a bit::matrix.
    auto T = xso::transition_matrix<State>();

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

    // stopwatch to time the two versions
    utilities::stopwatch sw;

    // Compute lots of random numbers using the transition matrix approach
    sw.click();
    for (std::size_t i = 0; i < n_trials; ++i) bits = bit::dot(T, bits);
    sw.click();
    auto t_secs = sw.lap();

    // Convert the final bit-vector back to a words so we can check them later.
    bits.export_bits(state);

    // Compute lots of random numbers in the usual manner.
    sw.click();
    for (std::size_t i = 0; i < n_trials; ++i) engine.step();
    sw.click();
    auto g_secs = sw.lap();

    // Grab the new engine state in words
    std::array<word_type, n_words> words;
    for (std::size_t i = 0; i < State::word_count(); ++i) words[i] = engine[i];

    // Check to see whether the two versions match.
    verify(state == words, "MISMATCH state = {}, words = {}", state, words);

    // All OK so print the timing information.
    double ratio = t_secs / g_secs;
    std::print("{:L} steps (matrix, step()) {:.2f}s, {:.6f}s => ratio = {:.0Lf}\n\n", n_trials, t_secs, g_secs, ratio);
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

    // Number of trials to run
    std::size_t n_trials = 20'000;

    // Print large numbers with commas
    utilities::pretty_print_thousands();

    run(x01, n_trials);
    run(x02, n_trials);
    run(x03, n_trials);
    run(x04, n_trials);
    run(x05, n_trials);
    run(x06, n_trials);
    run(x07, n_trials);
    run(x08, n_trials);
    run(x09, n_trials);
    run(x10, n_trials);
    run(x11, n_trials);
    run(x12, n_trials);
    run(x13, n_trials);
    run(x14, n_trials);
    run(x15, n_trials);
    run(x16, n_trials);
    run(x17, n_trials);

    return 0;
}
