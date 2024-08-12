/// @brief Compute some jump polynomial coefficients in word form for our xoshiro/xoroshiro engines.
/// @note  Uses the `bit` library.
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"
#include <bit/bit.h>

/// @brief Compute some jump polynomial coefficients in word form for one of our xoshiro/xoroshiro engines.
template<typename State>
void
compute_jump_words()
{
    // Print the name of the State class we are working on.
    std::print("Jump polynomials in words for: {}\n", State::xso_name());

    // Precompute the engine's characteristic bit::polynomial.
    auto c = xso::characteristic_polynomial<State>();

    // Some constants
    using word_type = typename State::word_type;
    constexpr std::size_t n_words = State::word_count();
    constexpr std::size_t n_bits = State::bit_count();

    // We will pack our jump polynomials into an array of words.
    using array_type = std::array<word_type, n_words>;
    array_type jump_words;

    // Jump amount: N = 2^(0.25*n_bits)
    std::size_t power = n_bits / 4;
    auto        jump_poly = xso::jump_polynomial(c, power, true).coefficients();
    jump_poly.export_bits(jump_words);
    std::print("jump25 #1: {::#x}\n", jump_words);

    // Jump amount: N = 2^(0.50*n_bits)
    power = n_bits / 2;
    jump_poly = xso::jump_polynomial(c, power, true).coefficients();
    jump_poly.export_bits(jump_words);
    std::print("jump50: {::#x}\n", jump_words);

    // Jump amount: N = 2^(0.75*n_bits)
    power = 3 * n_bits / 4;
    jump_poly = xso::jump_polynomial(c, power, true).coefficients();
    jump_poly.export_bits(jump_words);
    std::print("jump75: {::#x}\n", jump_words);
}

int
main()
{
    // Compute the jump polynomials for our state-engines of interest
    compute_jump_words<xso::xoshiro_4x32>();
    compute_jump_words<xso::xoshiro_4x64>();
    compute_jump_words<xso::xoshiro_8x64>();
    compute_jump_words<xso::xoroshiro_2x32>();
    compute_jump_words<xso::xoroshiro_2x64>();
    compute_jump_words<xso::xoroshiro_2x64b>();
    compute_jump_words<xso::xoroshiro_16x64>();
    return 0;
}
