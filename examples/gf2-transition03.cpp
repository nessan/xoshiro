// When is faster to use the transition matrix to jump ahead in a stream vs. just running the rng.step() method and
// discarding the result until we get to where we want to be.
//
// This uses the `gf2` library and should be run in release mode.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>
#include <utilities/utilities.h>
#include <gf2/gf2.h>

// When we get to O(n_bits^3) discards the transition matrix multiply starts to beat the simple discard().
template<typename RNG>
void
run(RNG& rng) {
    // Print the name of the rng we are working on.
    std::print("{}\n", rng);

    // Some constants etc.
    using word_type = typename RNG::word_type;
    using array_type = typename RNG::array_type;
    constexpr auto n_words = RNG::word_count();
    constexpr auto n_bits = RNG::bit_count();

    // Copy the current state to a bit-vector.
    gf2::BitVector<word_type> bits{n_bits};
    for (auto i = 0uz; i < n_words; ++i) bits.set_word(i, rng[i]);;

    // Optimal point is some factor of the state bit count.
    std::size_t n_discard = 8 * n_bits * n_bits * n_bits;

    // stopwatch to time the two versions
    utilities::stopwatch sw;

    // Version 1: Raise the transition matrix to the appropriate power to get ahead in the stream.
    sw.click();
    auto T = xso::transition_matrix<RNG>();
    T = T.to_the(n_discard);
    bits = T * bits;
    sw.click();
    auto t_secs = sw.lap();

    // Convert the bit-vector version back to word form so we can compare things later.
    array_type state;
    bits.to_words(state.begin());

    // Version 2: Naively discard generated numbers until we get to where we need to be in the stream.
    sw.click();
    for (auto i = 0uz; i < n_discard; ++i) rng.step();
    sw.click();
    auto g_secs = sw.lap();

    // Grab the new rng state in words
    array_type words;
    rng.get_state(words.begin());

    // Check to see whether the two versions match.
    always_confirm(state == words, "MISMATCH state = {}, words = {}", state, words);

    // All OK so print the timing information.
    double ratio = t_secs / g_secs;
    std::print("{:L} discards (T^n, step()) {:.2f}s, {:.4f}s => ratio = {:.1Lf}\n\n", n_discard, t_secs, g_secs, ratio);
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

    std::apply([&](auto&... rng) { (run(rng), ...); }, generators);
    return 0;
}
