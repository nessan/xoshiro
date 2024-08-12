/// @brief Basic check on the production of random samples.
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"
#include <numeric>

int
main()
{
    xso::rng    gen;
    std::size_t n, n_samples = 3;

    std::print("Integers from [1,10]\n");
    for (n = 0; n < n_samples; ++n) std::print("{} ", gen.sample(1, 10));
    std::print("\n");

    std::print("Reals from[1,10)\n");
    for (n = 0; n < n_samples; ++n) std::print("{:4.2f} ", gen.sample(1., 10.));
    std::print("\n");

    std::normal_distribution nd{70., 15.};
    std::print("Normals with mean {} and std-dev {}\n", nd.mean(), nd.stddev());
    for (n = 0; n < n_samples; ++n) std::print("{:4.2f} ", gen.sample(nd));
    std::print("\n");

    std::binomial_distribution bd{6, 0.5};
    std::print("Binomials with {} trials and P[success] = {}\n", bd.t(), bd.p());
    for (n = 0; n < n_samples; ++n) std::print("{} ", gen.sample(bd));
    std::print("\n");

    std::array<int, 10> array;
    std::iota(array.begin(), array.end(), 0);
    std::print("Random from the array {}\n", array);
    for (n = 0; n < n_samples; ++n) std::print("{} ", gen.sample(array));
    std::print("\n");

    return 0;
}