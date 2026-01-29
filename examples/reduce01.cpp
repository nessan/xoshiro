// Check that the `xso::internal::reduce` function matches the gf2::BitPolynomial::reduce_x_to_the` method.
// This uses the `gf2` library.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>
#include <utilities/utilities.h>
#include <gf2/gf2.h>

template<typename State>
void
check() {
    // What are we working on?
    std::print("Checking polynomial reduction for {}:\n", State::xso_name());

    using word_type = State::word_type;
    constexpr std::size_t n_words = State::word_count();
    constexpr std::size_t n_bits = State::bit_count();

    // The State has a characteristic polynomial c(x) = x^n_bits + p(x) where degree[p] < n_bits.
    // It will have the `n_bits` coefficients of p(x) available in a compact word format.
    std::array<word_type, n_words> p_words;
    State::characteristic_coefficients(p_words.begin());

    // We turn those words into a bit-vector.
    auto p_coeffs = gf2::BitVector<word_type>::from(p_words.begin(), p_words.end());

    // Then convert the coefficients into a bit-polynomial ...
    gf2::BitPolynomial<word_type> p{p_coeffs};

    // The full characteristic polynomial c(x) is x^n_bits + p(x).
    auto c = gf2::BitPolynomial<word_type>::x_to_the(n_bits) + p;

    // Some storage arrays we will use to store the modular reduction results.
    std::array<word_type, n_words> xso_r, gf2_r;
    xso_r.fill(0);
    gf2_r.fill(0);

    // Compute x^J mod c(x) or possibly x^(2^J) mod c(x) two ways
    bool        J_is_pow2 = false;
    std::size_t J = 0;
    std::size_t dJ = 17;

    // Run the two `reduce` methods over a range of jump sizes:
    std::size_t n_trials = 10'000;
    for (std::size_t i = 0; i < n_trials; ++i, J += (i + 1) * dJ) {

        // First use the xso::internal::reduce function which will hand back r(x) in compact word form.
        xso_r = xso::internal::reduce(p_words, J, J_is_pow2);

        // Ask c(x) to compute r(x) as a bit-polynomial and convert that to word-form.
        c.reduce_x_to_the(J, J_is_pow2).coefficients().to_words(gf2_r.begin());

        // Error out if the two results don't match.
        if(xso_r != gf2_r) {
            std::println();
            std::println("The two reduction methods do not match for jump J = {:L}.", J);
            std::println("xso::internal::reduce returned:      {::#x}", xso_r);
            std::println("gf2::BitPolynomial::reduce returned: {::#x}", gf2_r);
            exit(1);
        }
    }

    // Get to here if they all match.
    std::print("The two reduction methods MATCHED for all {:L} jumps!\n\n", n_trials);
}

int
main() {
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
