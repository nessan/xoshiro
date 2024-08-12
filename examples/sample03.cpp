/// @brief Statistical check on the production of random uniforms/variates samples.
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"

int main()
{
    utilities::pretty_print_thousands();

    xso::rng gen;
    std::size_t n_trials = 6'000'000;
    std::size_t count = 0;
    for(std::size_t n = 0; n < n_trials; ++n) if(gen.flip()) count++;
    std::print("{:L} heads from {:L} fair coin flips.\n", count, n_trials);

    count = 0;
    for(std::size_t n = 0; n < n_trials; ++n) if(gen.roll() == 4) count++;
    std::print("{:L} 'fours' from {:L} fair six-sided dice rolls.\n", count, n_trials);
}
