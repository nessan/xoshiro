// Compute some jump polynomial coefficients in word form for our xoshiro/xoroshiro state engines.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <xoshiro.h>
#include <gf2/gf2.h>

template<typename State>
void
run(State&) {
    // Print the name of the state type we are working on.
    std::println("Jump coefficients for: {}", State::type_string());

    // Precompute the state's characteristic bit-polynomial.
    auto c = xso::characteristic_polynomial<State>();

    // We will pack the jump polynomial coefficients into an array of words.
    typename State::array_type jump_words;

    // Jump amount: N = 2^(0.25*n_bits) where the State has `n_bits` bits of state.
    std::size_t power = State::bit_count() / 4;
    xso::jump_polynomial<State>(c, power, true).coefficients().to_words(jump_words.begin());
    std::println("jump25 #1: {::#x}", jump_words);

        // Jump amount: N = 2^(0.50*n_bits) where the State has `n_bits` bits of state.
    power = State::bit_count() / 2;
    xso::jump_polynomial<State>(c, power, true).coefficients().to_words(jump_words.begin());
    std::println("jump50: {::#x}", jump_words);

    // Jump amount: N = 2^(0.75*n_bits) where the State has `n_bits` bits of state.
    power = 3 * State::bit_count() / 4;
    xso::jump_polynomial<State>(c, power, true).coefficients().to_words(jump_words.begin());
    std::println("jump75: {::#x}", jump_words);

    std::println();
}

int
main() {
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
