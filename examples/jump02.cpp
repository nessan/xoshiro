/// @brief For each of our type aliased state-engines compare jumping vs discarding for some fixed jump size n
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"

template<typename State>
void
run(State& engine, std::size_t J)
{
    // What are we working on?
    std::print("Jumping/discarding {:L} states for {}:\n", J, engine);

    // Make an exact copy of the input generator
    auto tmp = engine;

    // stopwatch to time jumping vs. discarding
    utilities::stopwatch sw;

    // Jumping ...
    sw.click();
    xso::jump(engine, xso::jump_coefficients<State>(J));
    sw.click();
    auto jump_secs = sw.lap();

    // Discarding ...
    sw.click();
    tmp.discard(J);
    sw.click();
    auto discard_secs = sw.lap();

    // Check the two generators are the same by looking at the next output of each.
    verify(engine() == tmp(), "MISMATCH for jump size {:L}", J);

    // All OK so rint the timing info.
    auto ratio = discard_secs / jump_secs;
    std::print("Times: jump = {:4.3f}s, discard = {:4.3f}s => ratio = {:3.1Lf}\n\n", jump_secs, discard_secs, ratio);
}

int
main()
{
    // Print large numbers with commas
    utilities::pretty_print_thousands();

    // Our generators
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

    // Number of states to jump
    std::size_t J = 500'000'000;

    // And off we go ...
    run(x01, J);
    run(x02, J);
    run(x03, J);
    run(x04, J);
    run(x05, J);
    run(x06, J);
    run(x07, J);
    run(x08, J);
    run(x09, J);
    run(x10, J);
    run(x11, J);
    run(x12, J);
    run(x13, J);
    run(x14, J);
    run(x15, J);
    run(x16, J);
    run(x17, J);

    return 0;
}
