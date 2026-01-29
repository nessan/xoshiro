// A timing check of the `xso::internal::reduce` function vs. the `gf2::BitPolynomial::reduce` method.
// We expect `xso::internal::reduce` to be a little faster as it makes some simplifying assumptions.
// This uses the `gf2` library & should be run in release mode.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>
#include <utilities/utilities.h>
#include <gf2/gf2.h>

/// @brief We will compute r(x) = x^J mod c(x) two ways where J = N or J = 2^N.
template<typename State>
void
check(std::size_t N, bool N_is_pow2 = false) {
    // What are we working on?
    std::print("Calling `reduce({:L}, {})` two ways for {}:\n", N, N_is_pow2, State::xso_name());

    // Some constants etc.
    using word_type = typename State::word_type;
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

    // Word arrays we will use to store the reduce(...) results.
    std::array<word_type, n_words> xso_r, gf2_r;
    xso_r.fill(0);
    gf2_r.fill(0);

    // Number of trials
    std::size_t n_trials = 100'000;

    // Stopwatch to time the two versions
    using stopwatch = utilities::stopwatch<>;
    stopwatch sw;

    // Run the xso::internal::reduce method
    sw.click();
    for (std::size_t i = 0; i < n_trials; ++i) { xso_r = xso::internal::reduce(p_words, N, N_is_pow2); }
    sw.click();
    auto xso_lap = stopwatch::format_seconds(sw.lap());

    // Run the gf2::BitPolynomial::reduce method
    sw.click();
    for (std::size_t i = 0; i < n_trials; ++i) {
        c.reduce_x_to_the(N, N_is_pow2).coefficients().to_words(gf2_r.begin());
    }
    sw.click();
    auto gf2_lap = stopwatch::format_seconds(sw.lap());

    // Print the timing info.
    std::print("{:L} reduce calls took (xso, gf2): {}, {}s\n\n", n_trials, xso_lap, gf2_lap);

    // Error out if the two results don't match.
    if (xso_r != gf2_r) {
        std::println();
        std::println("The two reduction methods do not match for jump N = {:L}.", N);
        std::println("xso::internal::reduce returned:      {::#x}", xso_r);
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
