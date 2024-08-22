/// @brief Runs a timing comparison between our xoshiro and the equivalent "C" versions from the author's website.
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"
#include "vigna.h"

/// @brief Compare one of our xoshiro/Xoroshiro classes with the equivalent "C" version in a timing test
/// Starting from the same state we run a large number of trials to compare timing between the new and original versions
/// @note This needs to be run with a reasonable level of compiler optimization to be meaningful.
template<typename New, typename Old>
void
compare(New& x, Old& c)
{
    // Print the names of the generators we are working on
    std::print("Comparing {} with {}\n", utilities::type(c), x);

    // Make sure we start both generators with identical states ...
    for (std::size_t i = 0; i < x.word_count(); ++i) c.s[i] = x[i];

    // Timing Test: Generate many samples from the two versions of the generator and see how long those runs take
    std::size_t n_trials = 1'000'000'000;

    // Space to store the samples
    std::uint64_t rc, rx;

    // stopwatch to time the two versions
    utilities::stopwatch sw;

    // Time the C version
    sw.click();
    for (std::size_t i = 0; i < n_trials; ++i) { rc = c.next(); }
    sw.click();
    auto c_secs = sw.lap();

    // Time the X version
    sw.click();
    for (std::size_t i = 0; i < n_trials; ++i) { rx = x(); }
    sw.click();
    auto x_secs = sw.lap();

    // Check to see whether the two versions match.
    verify(rc == rx, "Generator mismatch! rc = {}, rx = {}\n", rc, rx);

    // All OK so print the timing info.
    std::print("{:L} calls took (old, new): {:.2f}s, {:.2f}s\n\n", n_trials, c_secs, x_secs);
}

int
main()
{
    // Make those large generated random numbers at least somewhat readable.
    utilities::pretty_print_thousands();

    // Our versions of the generators
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

    // The original "C" versions of the generators wrapped in simple structs
    old::xoroshiro_2x32_star       c01;
    old::xoroshiro_2x32_star_star  c02;
    old::xoshiro_4x32_plus         c03;
    old::xoshiro_4x32_plus_plus    c04;
    old::xoshiro_4x32_star_star    c05;
    old::xoroshiro_2x64_plus       c06;
    old::xoroshiro_2x64_plus_plus  c07;
    old::xoroshiro_2x64_star_star  c08;
    old::xoshiro_4x64_plus         c09;
    old::xoshiro_4x64_plus_plus    c10;
    old::xoshiro_4x64_star_star    c11;
    old::xoshiro_8x64_plus         c12;
    old::xoshiro_8x64_plus_plus    c13;
    old::xoshiro_8x64_star_star    c14;
    old::xoroshiro_16x64_star      c15;
    old::xoroshiro_16x64_star_star c16;
    old::xoroshiro_16x64_plus_plus c17;

    // Run the comparisons ...
    compare(x01, c01);
    compare(x02, c02);
    compare(x03, c03);
    compare(x04, c04);
    compare(x05, c05);
    compare(x06, c06);
    compare(x07, c07);
    compare(x08, c08);
    compare(x09, c09);
    compare(x10, c10);
    compare(x11, c11);
    compare(x12, c12);
    compare(x13, c13);
    compare(x14, c14);
    compare(x15, c15);
    compare(x16, c16);
    compare(x17, c17);

    return 0;
}