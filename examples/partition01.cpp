// Basic check that the xso::partition class compiles etc.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>
#include <utilities/utilities.h>

/// @brief Partition the input rng stream into n pieces.  Run a few calls for each of those.
template<typename PRNG>
void
run(PRNG& rng, std::size_t n) {
    // What is the type of generator we are working with?
    std::print("Calls to {} partitioned into {} streams yields:\n", rng, n);

    // Partition the input rng stream into n non-overlapping sub-streams,
    xso::partition partition(rng, n);

    // From the partition object we can capture the sub-streams as generators seeded ahead in the rng parent stream.
    std::vector<PRNG> g(n);
    for (auto i = 0uz; i < n; ++i) g[i] = partition.next();

    // Make some calls to each of the sub-stream generators to make sure they do something.
    for (std::size_t c = 0; c < 10; ++c) {
        std::print("Call {}: ", c);
        for (auto i = 0uz; i < n; ++i) std::print("p{} -> {:26L}, ", i, g[i]());
        std::print("\n");
    }
    std::print("\n");
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

    std::print("Creating some sub-streams ...\n\n");
    std::size_t n_partitions = 5;
    std::apply([&](auto&... rng) { (run(rng, n_partitions), ...); }, generators);

    return 0;
}
