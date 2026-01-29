// For each of our type aliased state-engines we find the corresponding characteristic polynomial c(x)
// We also check that c(x) is monic so c(x) = x^m + p(x) where deg[p(x)] < m.
// Finally we print p(x) in word format.
//
// This uses the `gf2` library.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>
#include <utilities/utilities.h>
#include <gf2/gf2.h>

template<typename State>
void
characteristic_polynomial() {
    // Which engines are we working on?
    std::print("State: {}\n", State::xso_name());

    // Get the characteristic gf2::BitPolynomial c(x) & make sure it is monic.
    auto c = xso::characteristic_polynomial<State>();
    always_confirm(c.is_monic(), "Characteristic polynomial high coefficient is NOT 1");

    // Find p(x) where c(x) = x^n + p(x) and deg[p(x)] < n.
    auto p = c.sub(c.size() - 1);

    // Convert the coefficients of p(x) to an array of words.
    std::array<typename State::word_type, State::word_count()> p_words;
    p_words.fill(0);
    p.coefficients().to_words(p_words.begin());

    // Output in hex format.
    std::print("c(x) = x^{} + p(x) where the coefficients of p(x) in words are:\n{::#x}\n\n", c.degree(), p_words);
}

int
main() {
    // Compute the characteristic polynomials for our state-engines of interest
    characteristic_polynomial<xso::xoshiro_4x32>();
    characteristic_polynomial<xso::xoshiro_4x64>();
    characteristic_polynomial<xso::xoshiro_8x64>();
    characteristic_polynomial<xso::xoroshiro_2x32>();
    characteristic_polynomial<xso::xoroshiro_2x64>();
    characteristic_polynomial<xso::xoroshiro_2x64b>();
    characteristic_polynomial<xso::xoroshiro_16x64>();
    return 0;
}
