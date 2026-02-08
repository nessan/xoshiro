// We find the corresponding characteristic polynomial c(x) for each of our type-aliased state-engines.
// We also check that c(x) is monic so c(x) = x^m + p(x) where deg[p(x)] < m.
// Finally we print p(x) in word format.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>
#include <utilities/utilities.h>
#include <gf2/gf2.h>

template<typename State>
void
run(State&) {
    // Which state type are we working on?
    std::print("State: {}\n", State::type_string());

    // Get the characteristic bit-polynomial c(x) & make sure it is monic.
    auto c = xso::characteristic_polynomial<State>();
    always_confirm(c.is_monic(), "Characteristic polynomial is not monic!");

    // Find p(x) where c(x) = x^n + p(x) and deg[p(x)] < n.
    auto p = c.sub_polynomial(c.size() - 1);

    // Convert the coefficients of p(x) to an array of words.
    typename State::array_type p_words;
    p.coefficients().to_words(p_words.begin());

    // Output in hex format.
    std::print("c(x) = x^{} + p(x) where the coefficients of p(x) in words are:\n{::#x}\n\n", c.degree(), p_words);
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
