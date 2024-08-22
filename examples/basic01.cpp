/// @brief Run all the predefined type aliased xoshiro/xoroshiro generators through a few iterations.
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"

/// @brief Run a random number generator through a a few trials
template<typename RNG>
void
run(RNG& rng, std::size_t n_trials = 5)
{
    std::size_t result_bits = std::numeric_limits<typename RNG::result_type>::digits;
    std::print("{} calls to {} yields the following {}-bit unsigneds:\n", n_trials, rng, result_bits);
    for (std::size_t i = 0; i < n_trials; ++i) std::print("{}; ", rng());
    std::print("\n\n");
}

int
main()
{
    // All our type aliased generators
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

    std::print("Running the various xoshiro/xoroshiro generators through a few cycles ...\n\n");
    std::size_t n_trials = 5;
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
