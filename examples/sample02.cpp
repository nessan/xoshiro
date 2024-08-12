/// @brief Basic check on the production of random uniforms/variates samples.
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"
#include <numeric>

int
main()
{
    xso::rng gen;

    constexpr std::size_t N = 10;
    constexpr std::size_t K = N / 2;

    std::array<int, N> u;
    std::iota(u.begin(), u.end(), 0);
    std::print("Population: {}\n", u);

    std::array<int, K> u_samples;
    gen.sample(u, u_samples.begin(), u_samples.size());
    std::print("Samples:    {}\n", u_samples);

    std::normal_distribution nd{70., 15.};
    std::array<double, N>    v;
    gen.sample(nd, v.begin(), v.size());
    std::print("Population: {::4.2f}\n", v);

    std::array<double, K> v_samples;
    gen.sample(v, v_samples.begin(), v_samples.size());
    std::print("Samples:    {::4.2f}\n", v_samples);
}