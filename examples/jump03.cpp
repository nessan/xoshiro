/// @brief Timing comparison between our jumps and using the pre-canned ones in the equivalent "C" version.
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"
#include "vigna.h"

template<typename New, typename Old>
void
compare(New& x, Old& c)
{
    // Print the names of the generators we are working on
    std::print("Comparing {} with {}\n", utilities::type(c), x);

    // Some constants etc.
    constexpr std::size_t n_words = New::word_count();
    constexpr std::size_t n_bits = New::bit_count();
    ;

    // Make sure we start both generators with identical states ...
    for (std::size_t i = 0; i < n_words; ++i) c.s[i] = x[i];

    // stopwatch to time the two versions
    utilities::stopwatch sw;

    // The precomputed jumps for the "C" versions is 2^(n_bits/2).
    sw.click();
    c.jump();
    sw.click();
    auto old_secs = sw.lap();

    // Our version of the jump ...
    sw.click();
    xso::jump(x, xso::jump_coefficients<New>(n_bits / 2, true));
    sw.click();
    auto new_secs = sw.lap();

    // Check the two generators are still the same by looking at the next output of each.
    verify(x() == c.next(), "MISMATCH");

    // All OK so print the timing info -- expect the "C" versions to be faster as they have pre-canned jumps.
    // However, both versions will be very fast and our jumps can be anything not just the pre-canned ones.
    auto ratio = new_secs / old_secs;
    std::print("Times: (new, old) = ({:4.3f}s, {:4.3f}s) => ratio = {:3.0Lf}\n\n", new_secs, old_secs, ratio);
}

int
main()
{
    // Make those large generated random numbers at least somewhat readable.
    utilities::pretty_print_thousands();

    // Our versions of the generators
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