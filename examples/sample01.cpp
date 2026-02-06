// Basic check on the production of random samples.

// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>

int
main() {
    xso::rng    rng;
    std::size_t n, n_samples = 3;

    std::println("Characters from ['a','z']");
    for (n = 0; n < n_samples; ++n) std::print("'{:c}' ", rng.sample(std::uint8_t{'a'}, std::uint8_t{'z'}));
    std::print("\n");

    std::println("Integers from [1,10]");
    for (n = 0; n < n_samples; ++n) std::print("{} ", rng.sample(1, 10));
    std::print("\n");

    std::println("Reals from [1,10)");
    for (n = 0; n < n_samples; ++n) std::print("{:4.2f} ", rng.sample(1., 10.));
    std::print("\n");

    std::normal_distribution nd{70., 15.};
    std::println("Normals with mean {} and std-dev {}", nd.mean(), nd.stddev());
    for (n = 0; n < n_samples; ++n) std::print("{:4.2f} ", rng.sample(nd));
    std::print("\n");

    std::binomial_distribution bd{6, 0.5};
    std::println("Binomials with {} trials and P[success] = {}", bd.t(), bd.p());
    for (n = 0; n < n_samples; ++n) std::print("{} ", rng.sample(bd));
    std::print("\n");

    std::array<int, 10> array;
    std::iota(array.begin(), array.end(), 0);
    std::println("Random from the array {}", array);
    for (n = 0; n < n_samples; ++n) std::print("{} ", rng.sample(array));
    std::print("\n");

    return 0;
}
