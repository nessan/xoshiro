// For each of our type aliased RNG-engines compare jumping vs discarding for some fixed jump size n.
// We do some timing so this needs to be compiled with optimization enabled.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>
#include <utilities/utilities.h>

template<typename RNG>
void
run(RNG& engine, std::size_t J) {
    // What are we working on?
    std::print("Jumping/discarding {:L} random variates from {}:\n", J, engine);

    // Make an exact copy of the input generator
    auto tmp = engine;

    // stopwatch to time jumping vs. discarding
    utilities::stopwatch sw;

    // Jumping without a precomputed jump polynomial (so should be slower)...
    sw.click();
    engine.jump(J);
    sw.click();
    auto jump_secs = sw.lap();

    // Discarding ...
    sw.click();
    tmp.discard(J);
    sw.click();
    auto discard_secs = sw.lap();

    // Check the two generators are the same by looking at the next output of each.
    always_confirm(engine() == tmp(), "MISMATCH for jump size {:L}", J);

    // All OK so rint the timing info.
    auto ratio = discard_secs / jump_secs;
    std::print("Times: jump = {:4.3f}s, discard = {:4.3f}s => ratio = {:3.1Lf}\n\n", jump_secs, discard_secs, ratio);
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

    // Number of RNGs to jump
    std::size_t J = 500'000'000;
    std::apply([&](auto&... rng) { (run(rng, J), ...); }, generators);

    return 0;
}
