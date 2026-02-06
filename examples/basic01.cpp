// Run all the predefined type aliased xoshiro/xoroshiro generators through a few iterations.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>
#include <utilities/utilities.h>

/// @brief Run a random number generator through a a few trials
template<typename RNG>
void
run(RNG& rng, std::size_t n_trials = 5) {
    std::size_t result_bits = std::numeric_limits<typename RNG::result_type>::digits;
    std::print("{} calls to {} yields the following {}-bit unsigneds:\n", n_trials, rng, result_bits);
    for (auto i = 0uz; i < n_trials; ++i) std::print("{:L}; ", rng());
    std::print("\n\n");
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

    std::print("Running the various xoshiro/xoroshiro generators through a few cycles ...\n\n");
    std::size_t n_trials = 5;
    std::apply([&](auto&... rng) { (run(rng, n_trials), ...); }, generators);

    return 0;
}
