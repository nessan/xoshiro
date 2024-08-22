/// @brief Compare two ways of jumping one of our xoshiro/xoroshiro classes.
///        Using the jump polynomial vs. using T^n where T is the transition matrix.
/// @note  This function uses the @c bit library.
///
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"
#include "vigna.h"
#include <bit/bit.h>

template<typename State>
void
compare(State& x)
{
    // stopwatch to time the two versions
    utilities::stopwatch sw;

    // Print the name of the generator we are working on
    std::print("Working on {}:\n", x);

    // Some constants etc.
    using word_type = typename State::word_type;
    constexpr std::size_t n_words = State::word_count();
    constexpr std::size_t n_bits = State::bit_count();;

    // First consider jumping by N = 2^(n_bits/2) slots.
    std::size_t power = n_bits / 2;

    // Time how long it takes to compute the jump polynomial for that power of 2.
    sw.click();
    auto r = xso::jump_coefficients<State>(power, true);
    sw.click();
    auto polynomial_secs = sw.lap();
    std::print("Time to compute jump_polynomial({:3}):  {:<6.1Lf}ms\n", power, 1000 * polynomial_secs);

    // Time the alternative direct matrix approach where we raise the transition matrix to power N.
    sw.click();
    auto T = xso::transition_matrix<State>();
    T = bit::pow2(T, power);
    sw.click();
    auto matrix_secs = sw.lap();
    std::print("Time to compute bit::pow2(T, {:3}):     {:<6.1Lf}ms\n", power, 1000 * matrix_secs);
    std::print("Ratio:                                 {:<6.1Lf}\n", matrix_secs / polynomial_secs);

    // Need a copy of the generator to use for the matrix multiply approach.
    auto y = x;

    // Time how long it takes to make lots of jumps using the jump polynomial approach.
    std::size_t n_jumps = 1024;
    sw.click();
    for (std::size_t n = 0; n < n_jumps; ++n) xso::jump(x, r);
    sw.click();
    polynomial_secs = sw.lap();

    // Time how long it takes to make lots of jumps using the matrix multiply approach.
    sw.click();

    // Need some work storage to go between word-space and bit-space
    // Storage where we can go back and forth between bit-space and word-space.
    std::array<word_type, n_words> words;
    bit::vector                    bits{n_bits};

    // And off we go ...
    for (std::size_t n = 0; n < n_jumps; ++n) {
        for (std::size_t i = 0; i < n_words; ++i) words[i] = y[i];
        bits.import_bits(words);
        bits = bit::dot(T, bits);
        bits.export_bits(words);
        y.seed(words.cbegin(), words.cend());
    }
    sw.click();
    matrix_secs = sw.lap();

    // Check the two generators are still the same by looking at the next output of each.
    verify(x() == y(), "MISMATCH");

    // All OK so print the timing info on polynomial jumps vs matrix jumps ...
    std::print("Number of jump aheads performed:       {:<6L}\n", n_jumps);
    std::print("Polynomial method took:                {:<6.3Lf}ms\n", polynomial_secs);
    std::print("Matrix multiply method took:           {:<6.3Lf}ms\n", matrix_secs);
    std::print("Ratio or those two methods:            {:<6.1Lf}\n\n", matrix_secs / polynomial_secs);
}

int
main()
{
    // Make those large generated random numbers at least somewhat readable.
    utilities::pretty_print_thousands();

    // Our versions of the generators
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

    // Run the comparisons ...
    compare(x03);
    compare(x04);
    compare(x05);
    compare(x06);
    compare(x07);
    compare(x08);
    compare(x09);
    compare(x10);
    compare(x11);
    compare(x12);
    compare(x13);
    compare(x14);
    compare(x15);
    compare(x16);
    compare(x17);

    return 0;
}