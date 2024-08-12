/// @brief We check that the @c xso::internal::reduce function matches the @c bit::polynomial::reduce method.
/// @note  This uses the `bit` library.
///
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"
#include <bit/bit.h>

template<typename State>
void
check()
{
    // What are we working on?
    std::print("Checking {}:\n", State::xso_name());

    using word_type = State::word_type;
    constexpr std::size_t n_words = State::word_count();
    constexpr std::size_t n_bits = State::bit_count();
    ;

    // The State has a characteristic polynomial c(x) = x^n + p(x) where degree[p] < n.
    // It should have the n coefficients of p available in a compact word format.
    std::array<word_type, n_words> p_words;
    State::characteristic_coefficients(p_words.begin());

    // We turn those words into a bit::vector of coefficients and those into a bit::polynomial.
    bit::polynomial p{bit::vector{p_words}};

    // The full characteristic polynomial c(x) is x^n_bits + p(x).
    auto c = bit::polynomial<>::power(n_bits) + p;

    // Word arrays we will use to store the reduce(...) results.
    std::array<word_type, n_words> xso_r, bit_r;

    // Compute x^J mod c(x) or possibly x^(2^J) mod c(x) two ways
    bool        J_is_pow2 = false;
    std::size_t J = 0;
    std::size_t dJ = 17;

    // Run the two `reduce` methods over a range of jump sizes:
    std::size_t n_trials = 10'000;
    for (std::size_t i = 0; i < n_trials; ++i, J += (i + 1) * dJ) {

        // First use the xso::internal::reduce function which will hand back r(x) in compact word form.
        xso_r = xso::internal::reduce(p_words, J, J_is_pow2);

        // Use the bit::polynomial::reduce instance method to get r(x) as a bit::polynomial & export it in word form.
        c.reduce(J, J_is_pow2).coefficients().export_bits(bit_r);

        // Check the two results match.
        verify(xso_r == bit_r, "For jump J = {:L}.\nxso::reduce -> {::#x}\nbit::reduce -> {::#x}\n", J, xso_r, bit_r);
    }

    // The all match.
    std::print("The two reduction methods MATCH!\n\n");
}

int
main()
{
    // Make those large generated random numbers at least somewhat readable.
    utilities::pretty_print_thousands();

    // Check each engine of interest
    check<xso::xoshiro_4x32>();
    check<xso::xoshiro_4x64>();
    check<xso::xoshiro_8x64>();
    check<xso::xoroshiro_2x32>();
    check<xso::xoroshiro_2x64>();
    check<xso::xoroshiro_2x64b>();
    check<xso::xoroshiro_16x64>();
    return 0;
}
