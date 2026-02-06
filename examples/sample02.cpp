// Basic check on the production of random uniforms/variates samples.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>
#include <numeric>

int
main() {
    xso::rng rng;

    constexpr std::size_t N = 10;
    constexpr std::size_t K = N / 2;

    std::array<int, N> u;
    std::iota(u.begin(), u.end(), 0);

    std::array<int, K> u_samples;
    rng.sample(u, u_samples.begin(), K);
    std::println("Array of values:           {}", u);
    std::println("Samples from array:        {}", u_samples);

    std::normal_distribution nd{70., 15.};

    std::array<double, K> nd_samples;
    rng.sample(nd, nd_samples.begin(), K);
    std::println("Samples from distribution: {::4.2f}", nd_samples);
}