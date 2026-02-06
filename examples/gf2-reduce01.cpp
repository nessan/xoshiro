// Check that the `xso::reduce` function matches the gf2::BitPolynomial::reduce_x_to_the` method.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>
#include <utilities/utilities.h>
#include <gf2/gf2.h>

template<typename State>
void
run(State&) {
    // What are we working on?
    std::print("Checking polynomial reduction for {}:\n", State::type_string());

    using word_type = State::word_type;

    // The RNG has a characteristic polynomial c(x) = x^n_bits + p(x) where degree[p] < n_bits.
    // It will have the coefficients of p(x) available in a compact word format which we can copy here.
    auto p_words = State::characteristic_coefficients();

    // Convert the p(x) coefficients into a bit-polynomial and add an x^n_bits term to form c(x).
    gf2::BitPolynomial<word_type> c{gf2::BitVector<word_type>::from(p_words.begin(), p_words.end())};
    c += gf2::BitPolynomial<word_type>::x_to_the(State::bit_count());

    // Some storage arrays we will use to store the modular reduction results.
    typename State::array_type xso_r, gf2_r;
    xso_r.fill(0);
    gf2_r.fill(0);

    // Compute x^J mod c(x) or possibly x^(2^J) mod c(x) two ways
    auto J_is_log2 = false;
    auto J = 0uz;
    auto dJ = 17uz;

    // Run the two `reduce` methods over a range of jump sizes:
    auto n_trials = 10'000uz;
    for (auto i = 0uz; i < n_trials; ++i, J += (i + 1) * dJ) {

        // First use the internal function which will hand back r(x) in compact word form.
        xso_r = xso::reduce(p_words, J, J_is_log2);

        // Use the `gf2` library and ask c(x) to compute r(x) as a bit-polynomial and convert that to word-form.
        c.reduce_x_to_the(J, J_is_log2).coefficients().to_words(gf2_r.begin());

        // Error out if the two results don't match.
        if (xso_r != gf2_r) {
            std::println();
            std::println("The two reduction methods do not match for jump J = {:L}.", J);
            std::println("xso::reduce returned:                {::#x}", xso_r);
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

    // clang-format off
    // One variant of each type-aliased state-engine (different types, so stored in a tuple)
    auto states = std::tuple{
        xso::xoshiro_4x32{},
        xso::xoshiro_4x64{},
        xso::xoshiro_8x64{},
        xso::xoroshiro_2x32{},
        xso::xoroshiro_2x64{},
        xso::xoroshiro_2x64b{},
        xso::xoroshiro_16x64{}
    };
    // clang-format on

    std::apply([&](auto&... state) { (run(state), ...); }, states);

    return 0;
}
