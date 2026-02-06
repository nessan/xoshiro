// Compare two ways of jumping the xoshiro/xoroshiro classes: jump-polynomial vs T^n where T is the transition matrix.
// This uses the `gf2` library and should be run in release mode.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>
#include <utilities/utilities.h>
#include <gf2/gf2.h>
#include "vigna.h"

template<typename RNG>
void
run(RNG& rng) {
    // stopwatch to time the two versions
    utilities::stopwatch sw;

    // Print the name of the generator we are working on
    std::print("Working on {}:\n", rng);

    // Some constants etc.
    using word_type = typename RNG::word_type;
    using array_type = typename RNG::array_type;
    constexpr auto n_bits = RNG::bit_count();

    // First consider jumping by N = 2^(n_bits/2) slots.
    std::size_t power = n_bits / 2;

    // Time how long it takes to compute the jump polynomial for that power of 2.
    sw.click();
    auto r = xso::jump_coefficients<typename RNG::state_type>(power, true);
    sw.click();
    auto polynomial_secs = sw.lap();
    std::print("Time to compute jump_polynomial({:<3}):  {:<6.1Lf}ms\n", power, 1000 * polynomial_secs);

    // Time the alternative direct matrix approach where we raise the transition matrix to power N.
    sw.click();
    auto T = xso::transition_matrix<RNG>();
    T = T.to_the(power, true);
    sw.click();
    auto matrix_secs = sw.lap();
    std::print("Time to compute  T^2^{:<3}:              {:<6.1Lf}ms\n", power, 1000 * matrix_secs);
    std::print("Ratio:                                 {:<6.0Lf}\n", matrix_secs / polynomial_secs);

    // Need a copy of the generator to use for the matrix multiply approach.
    auto rng_copy = rng;

    // Time how long it takes to make lots of jumps using the jump polynomial approach.
    std::size_t n_jumps = 1024;
    sw.click();
    for (std::size_t n = 0; n < n_jumps; ++n) rng.jump(r);
    sw.click();
    polynomial_secs = sw.lap();


    // Need some work storage to go between word-space and bit-space
    // Storage where we can go back and forth between bit-space and word-space.
    array_type words;
    gf2::BitVector<word_type> bits{n_bits};

    // Time how long it takes to make lots of jumps using the matrix multiply approach.
    sw.click();
    for (std::size_t n = 0; n < n_jumps; ++n) {
        rng_copy.get_state(words.begin());
        bits.copy(words.begin(), words.end());
        bits = T * bits;
        bits.to_words(words.begin());
        rng_copy.seed(words.cbegin(), words.cend());
    }
    sw.click();
    matrix_secs = sw.lap();

    // All OK so print the timing info on polynomial jumps vs matrix jumps ...
    std::print("Number of jump aheads performed:       {:<6L}\n", n_jumps);
    std::print("Polynomial method took:                {:<6.3Lf}ms\n", polynomial_secs);
    std::print("Matrix multiply method took:           {:<6.3Lf}ms\n", matrix_secs);
    std::print("Ratio or those two methods:            {:<6.1Lf}\n\n", matrix_secs / polynomial_secs);

    // Check the two generators are still the same by looking at the next output of each.
    always_confirm(rng() == rng_copy(), "MISMATCH");
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
