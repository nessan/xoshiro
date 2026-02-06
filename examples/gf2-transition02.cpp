// Just how slow is it to generate random numbers from a transition matrix?
// The test should be run in release mode for meaningful timings.
//
// This uses the `gf2` library.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>
#include <utilities/utilities.h>
#include <gf2/gf2.h>

// Time how long the transition matrix approach takes versus the conventional step() method.
template<typename RNG>
void
run(RNG& rng, std::size_t n_trials) {
    // Print the name of the generator we are working on
    std::print("{}\n", rng);

    // Get the transition matrix for this RNG as a gf2::BitMatrix.
    auto T = xso::transition_matrix<RNG>();

    // Some constants etc.
    using word_type = typename RNG::word_type;
    using array_type = typename RNG::array_type;
    constexpr auto n_words = RNG::word_count();
    constexpr auto n_bits = RNG::bit_count();

    // Copy the current state to a bit-vector.
    gf2::BitVector<word_type> bits{n_bits};
    for (auto i = 0uz; i < n_words; ++i) bits.set_word(i, rng[i]);

    // stopwatch to time the two versions
    utilities::stopwatch sw;

    // Compute lots of random numbers using the transition matrix approach
    sw.click();
    for (auto i = 0uz; i < n_trials; ++i) bits = gf2::dot(T, bits);
    sw.click();
    auto t_secs = sw.lap();

    // Convert the final bit-vector back to a words so we can check them later.
    array_type state;
    bits.to_words(state.begin());

    // Compute lots of random numbers in the usual manner.
    sw.click();
    for (auto i = 0uz; i < n_trials; ++i) rng.step();
    sw.click();
    auto g_secs = sw.lap();

    // Grab the rng state as an array of words
    array_type words;
    for (auto i = 0uz; i < RNG::word_count(); ++i) words[i] = rng[i];

    // Check to see whether the two versions match.
    always_confirm(state == words, "MISMATCH state = {}, words = {}", state, words);

    // All OK so print the timing information.
    double ratio = t_secs / g_secs;
    std::print("{:L} steps (matrix, step()) {:.2f}s, {:.6f}s => ratio = {:.0Lf}\n\n", n_trials, t_secs, g_secs, ratio);
}

int
main() {
    utilities::pretty_print_thousands();

    // clang-format off
    // All our type aliased generators (different types, so stored in a tuple)
    auto generators = std::tuple{
        xso::xoroshiro_2x32_star{},
        xso::xoroshiro_2x32_star_star{},
        xso::xoshiro_4x32_plus{},
        xso::xoshiro_4x32_plus_plus{},
        xso::xoshiro_4x32_star_star{},
        xso::xoroshiro_2x64_plus{},
        xso::xoroshiro_2x64_plus_plus{},
        xso::xoroshiro_2x64_star_star{},
        xso::xoshiro_4x64_plus{},
        xso::xoshiro_4x64_plus_plus{},
        xso::xoshiro_4x64_star_star{},
        xso::xoshiro_8x64_plus{},
        xso::xoshiro_8x64_plus_plus{},
        xso::xoshiro_8x64_star_star{},
        xso::xoroshiro_16x64_star{},
        xso::xoroshiro_16x64_star_star{},
        xso::xoroshiro_16x64_plus_plus{},
    };
    // clang-format on

    std::size_t n_trials = 200'000;
    std::apply([&](auto&... rng) { (run(rng, n_trials), ...); }, generators);
    return 0;
}
