/// @brief A timing check of the @c xso::reduce function vs. the @c bit::polynomial::reduce method.
/// @note  We expect @b xso::reduce to be a little faster as it makes some simplifying assumptions.
/// @note  This uses the `bit` library.
///
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"
#include <bit/bit.h>

/// @brief We will compute r(x) = x^J mod c(x) two ways where J = N or J = 2^N.
template<typename State>
void
check(std::size_t N, bool N_is_pow2 = false)
{
    // What are we working on?
    std::print("Calling `reduce({:L}, {})` two ways for {}:\n", N, N_is_pow2, State::xso_name());

    // Some constants etc.
    using word_type = typename State::word_type;
    constexpr std::size_t n_words = State::word_count();
    constexpr std::size_t n_bits = State::bit_count();
    ;

    // The State has a characteristic polynomial c(x) = x^n + p(x) where degree[p] < n.
    // It should have the n coefficients of p available in a compact word format.
    std::array<word_type, n_words> p_words;
    State::characteristic_coefficients(p_words.begin());

    // We turn those words into a bit::vector of coefficients and those into a bit::polynomial.
    bit::polynomial p{bit::vector{p_words}};

    // We can recreate c(x) as a bit::polynomial.
    auto c = bit::polynomial<>::power(n_bits) + p;

    // Word arrays we will use to store the reduce(...) results.
    std::array<word_type, n_words> xso_r, bit_r;

    // Number of trials
    std::size_t n_trials = 100'000;

    // Stopwatch to time the two versions
    utilities::stopwatch sw;

    // Run the xso::internal::reduce method
    sw.click();
    for (std::size_t i = 0; i < n_trials; ++i) { xso_r = xso::internal::reduce(p_words, N, N_is_pow2); }
    sw.click();
    auto xso_secs = sw.lap();

    // Run the bit::polynomial::reduce method
    sw.click();
    for (std::size_t i = 0; i < n_trials; ++i) { c.reduce(N, N_is_pow2).coefficients().export_bits(bit_r); }
    sw.click();
    auto bit_secs = sw.lap();

    // Check the two results match.
    verify(xso_r == bit_r, "For jump N = {:L}.\nxso::reduce -> {::#x}\nbit::reduce -> {::#x}\n", N, xso_r, bit_r);

    // All OK so print the timing info.
    std::print("{:L} reduce calls took (xso, bit): {:.2f}s, {:.2f}s\n\n", n_trials, xso_secs, bit_secs);
}

int
main()
{
    // Make those large generated random numbers at least somewhat readable.
    utilities::pretty_print_thousands();

    // We will compute r(x) = x^J mod c(x) two ways where J = N or J = 2^N.
    bool        N_is_pow2 = false;
    std::size_t N = 41234141;

    // Check each engine of interest
    check<xso::xoshiro_4x32>(N, N_is_pow2);
    check<xso::xoshiro_4x64>(N, N_is_pow2);
    check<xso::xoshiro_8x64>(N, N_is_pow2);
    check<xso::xoroshiro_2x32>(N, N_is_pow2);
    check<xso::xoroshiro_2x64>(N, N_is_pow2);
    check<xso::xoroshiro_2x64b>(N, N_is_pow2);
    check<xso::xoroshiro_16x64>(N, N_is_pow2);
    return 0;
}
