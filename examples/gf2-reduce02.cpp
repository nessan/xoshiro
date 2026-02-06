// A timing check of the `xso::reduce` function vs. the `gf2::BitPolynomial::reduce` method.
// We expect `xso::reduce` to be a little faster as it makes some simplifying assumptions.
// Should be run in release mode.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>
#include <utilities/utilities.h>
#include <gf2/gf2.h>

/// We will compute r(x) = x^J mod c(x) two ways where J = N or J = 2^N.
template<typename State>
void
check(State&, std::size_t N, bool N_is_pow2 = false) {
    // What are we working on?
    std::print("Calling `reduce({:L}, {})` two ways for {}:\n", N, N_is_pow2, State::type_string());

    using word_type = typename State::word_type;

    // The State has a characteristic polynomial c(x) = x^n_bits + p(x) where degree[p] < n_bits.
    // It will have the `n_bits` coefficients of p(x) available in a compact word format.
    auto p_words = State::characteristic_coefficients();

    // Convert the p(x) coefficients into a bit-polynomial and add an x^n_bits term to form c(x).
    gf2::BitPolynomial<word_type> c{gf2::BitVector<word_type>::from(p_words.begin(), p_words.end())};
    c += gf2::BitPolynomial<word_type>::x_to_the(State::bit_count());

    // Word arrays we will use to store the reduce(...) results.
    typename State::array_type xso_r, gf2_r;
    xso_r.fill(0);
    gf2_r.fill(0);

    // Number of trials
    std::size_t n_trials = 100'000;

    // Stopwatch to time the two versions
    using stopwatch = utilities::stopwatch<>;
    stopwatch sw;

    // Run the xso::reduce method
    sw.click();
    for (auto i = 0uz; i < n_trials; ++i) { xso_r = xso::reduce(p_words, N, N_is_pow2); }
    sw.click();
    auto xso_lap = stopwatch::format_seconds(sw.lap());

    // Run the gf2::BitPolynomial::reduce method
    sw.click();
    for (auto i = 0uz; i < n_trials; ++i) { c.reduce_x_to_the(N, N_is_pow2).coefficients().to_words(gf2_r.begin()); }
    sw.click();
    auto gf2_lap = stopwatch::format_seconds(sw.lap());

    // Print the timing info.
    std::print("{:L} reduce calls took (xso, gf2): {}, {}s\n\n", n_trials, xso_lap, gf2_lap);

    // Error out if the two results don't match.
    if (xso_r != gf2_r) {
        std::println();
        std::println("The two reduction methods do not match for jump N = {:L}.", N);
        std::println("xso::reduce returned:                {::#x}", xso_r);
        std::println("gf2::BitPolynomial::reduce returned: {::#x}", gf2_r);
        exit(1);
    }
}

int
main() {
    utilities::pretty_print_thousands();

    // We will compute r(x) = x^J mod c(x) two ways where J = N or J = 2^N.
    bool        N_is_pow2 = false;
    std::size_t N = 41'234'141;

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

    // Check each of them
    std::apply([&](auto&... state) { (check(state, N, N_is_pow2), ...); }, states);
    return 0;
}
