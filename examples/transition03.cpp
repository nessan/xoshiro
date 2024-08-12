/// @brief When is faster to use the transition matrix to jump ahead in a stream vs. just running the engine.step()
/// method and discarding the result until we get to where we want to be.
/// @note  Uses the `bit` library.
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"
#include <bit/bit.h>

/// @brief When we get to O(n_bits^3) discards the transition matrix multiply starts to beat the simple discard().
template<typename State>
void
run(State& engine)
{
    // Print the name of the engine we are working on.
    std::print("{}\n", engine);

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

    // Optimal point is some factor of the state bit count.
    std::size_t n_discard = 8 * n_bits * n_bits * n_bits;

    // stopwatch to time the two versions
    utilities::stopwatch sw;

    // Version 1: Raise the transition matrix to the appropriate power to get ahead in the stream.
    sw.click();
    auto T = xso::transition_matrix<State>();
    T = bit::pow(T, n_discard);
    bits = bit::dot(T, bits);
    sw.click();
    auto t_secs = sw.lap();

    // Convert the bit-vector version back to word form so we can compare things later.
    bits.export_bits(state);

    // Version 2: Naively discard generated numbers until we get to where we need to be in the stream.
    sw.click();
    for (std::size_t i = 0; i < n_discard; ++i) engine.step();
    sw.click();
    auto g_secs = sw.lap();

    // Grab the new engine state in words
    std::array<word_type, n_words> words;
    for (std::size_t i = 0; i < State::word_count(); ++i) words[i] = engine[i];

    // Check to see whether the two versions match.
    verify(state == words, "MISMATCH state = {}, words = {}", state, words);

    // All OK so print the timing information.
    double ratio = t_secs / g_secs;
    std::print("{:L} discards (T^n, step()) {:.2f}s, {:.4f}s => ratio = {:.1Lf}\n\n", n_discard, t_secs, g_secs, ratio);
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

    run(x01);
    run(x02);
    run(x03);
    run(x04);
    run(x05);
    run(x06);
    run(x07);
    run(x08);
    run(x09);
    run(x10);
    run(x11);
    run(x12);
    run(x13);
    run(x14);
    run(x15);
    run(x16);
    run(x17);

    return 0;
}
