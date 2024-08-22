/// @brief Compute a range of jump polynomials in word form for one of our xoshiro/xoroshiro engines.
/// @note  Uses the `bit` library.
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"
#include <bit/bit.h>

/// @brief Print a std::array of unsigned integers in a hex format surrounded by braces.
template<typename T, std::size_t N>
void
hex_print_array(std::array<T, N>& v)
{
    std::cout << "{";
    for (std::size_t i = 0; i < N; ++i) {
        std::print("{:#18x}", v[i]);
        if (i + 1 < N) std::print(", ");
    }
    std::cout << "}";
}

/// @brief Precompute some standard jump polynomials in word form for one of our xoshiro/xoroshiro engines.
/// If the engine has n_bits of state then its period is 2^n_bits (well 2^n_bits - 1 really).
/// We want to chunk that orbit up into N equal sized non-overlapping sub-streams where N = 2, 2^2, 2^3, ..., 2^20.
/// The first sub-stream starts at s0, the next at s1 etc.  Our jump polynomials get you from s0 to s1 ...
/// The polynomials are output in a format suitable for pasting into a C++ header file.
template<typename State>
void
compute_jump_words(std::size_t n_lo, std::size_t n_hi)
{
    // Print the name of the State class we are working on.
    std::print("Jump polynomials in words for: {}\n", State::xso_name());

    // Precompute the engine's characteristic bit::polynomial.
    auto c = xso::characteristic_polynomial<State>();

    // How many bits are there in the state?
    using word_type = typename State::word_type;
    constexpr std::size_t n_words = State::word_count();
    constexpr std::size_t n_bits = State::bit_count();

    // We will pack our jump polynomials into an array of words.
    using array_type = std::array<word_type, n_words>;
    array_type jump_words;

    // Dump the array of jump word array in a form that can be used in a C++ header file.
    std::cout << "static constexpr word_type jump_polynomial[] = {\n";

    // Compute the individual jump polynomials ...
    for (std::size_t n = n_lo; n <= n_hi; ++n) {

        // Jump amount: N = 2^n_bits / 2^n = 2^(n_bits - n)
        std::size_t power_two = n_bits - n;
        auto        jump_poly = xso::jump_polynomial(c, power_two, true).coefficients();
        jump_poly.export_bits(jump_words);

        // Dump this jump polynomial as one element of the entire array of jump polynomials
        std::cout << "    ";
        hex_print_array(jump_words);
        if (n + 1 <= n_hi) std::cout << ",";
        std::cout << '\n';
    }

    // Close out the big array here.
    std::cout << "};\n";
}

int
main()
{
    // Compute some jump polynomials for our type-aliases specific state-engines.
    // For parallel processing we wish to split the orbit of the state-engine into equal sized non-overlapping
    // sub-streams. So one full stream starting at state s0 becomes say 2 sub-stream where the first starts at s0 and
    // the second at s1 where s1 is just s0 jumped half way along the full stream. Of course we might want 4
    // sub-streams, or 8, or 16, ... or 1,000,000 sub-streams which is roughly 2^20.
    std::size_t lo_power_two = 1;
    std::size_t hi_power_two = 20;
    compute_jump_words<xso::xoshiro_4x32>(lo_power_two, hi_power_two);
    compute_jump_words<xso::xoshiro_4x64>(lo_power_two, hi_power_two);
    compute_jump_words<xso::xoshiro_8x64>(lo_power_two, hi_power_two);
    compute_jump_words<xso::xoroshiro_2x32>(lo_power_two, hi_power_two);
    compute_jump_words<xso::xoroshiro_2x64>(lo_power_two, hi_power_two);
    compute_jump_words<xso::xoroshiro_2x64b>(lo_power_two, hi_power_two);
    compute_jump_words<xso::xoroshiro_16x64>(lo_power_two, hi_power_two);
    return 0;
}
