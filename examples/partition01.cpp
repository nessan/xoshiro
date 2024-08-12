/// @brief Basic check that the xso::partition class compiles etc.
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"

/// @brief Partition the input rng stream into n pieces.  Run a few calls for each of those.
template<typename PRNG>
void
run(PRNG& rng, std::size_t n)
{
    // What is the type of generator we are working with?
    std::print("Calls to {} partitioned into {} streams yields:\n", rng, n);

    // Partition the input rng stream into n non-overlapping sub-streams,
    xso::partition partition(rng, n);

    // From the partition object we can capture the sub-streams as generators seeded ahead in the rng parent stream.
    std::vector<PRNG> g(n);
    for (std::size_t i = 0; i < n; ++i) g[i] = partition.next();

    // Make some calls to each of the sub-stream generators to make sure they do something.
    for (std::size_t c = 0; c < 10; ++c) {
        std::print("Call {}: ", c);
        for (std::size_t i = 0; i < n; ++i) std::print("p{} -> {:26L}, ", i, g[i]());
        std::print("\n");
    }
    std::print("\n");
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

    // Make the large generated numbers somewhat readable
    utilities::pretty_print_thousands();

    std::print("Creating some sub-streams ...\n\n");
    std::size_t n_partitions = 5;
    run(x01, n_partitions);
    run(x02, n_partitions);
    run(x03, n_partitions);
    run(x04, n_partitions);
    run(x05, n_partitions);
    run(x06, n_partitions);
    run(x07, n_partitions);
    run(x08, n_partitions);
    run(x09, n_partitions);
    run(x10, n_partitions);
    run(x11, n_partitions);
    run(x12, n_partitions);
    run(x13, n_partitions);
    run(x14, n_partitions);
    run(x15, n_partitions);
    run(x16, n_partitions);
    run(x17, n_partitions);

    return 0;
}
