/// @brief Create a randomly seeded RNG from a std::seed_seq
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"

/// @brief Returns a std::seed_seq that itself is seeded with an appropriate amount of entropy.
/// @param state_bits The number of bits in the generator's state array you are trying to seed.
std::seed_seq
seed_sequence(std::size_t state_bits)
{
    // We will use calls to std::random_device to produce the entropy.
    std::random_device dev;

    // Each call to dev() produces a certain number of bits of entropy
    using dev_output_type = typename std::random_device::result_type;
    std::size_t bits_per_call = std::numeric_limits<dev_output_type>::digits;

    // So for state_bits we will generally need to call dev() multiple times
    std::size_t calls_needed = (state_bits + bits_per_call - 1) / bits_per_call;

    // Fill an array with that number of calls to dev()
    std::vector<dev_output_type> dev_words(calls_needed);
    std::generate(dev_words.begin(), dev_words.end(), [&dev] { return dev(); });

    // Use that to seed a std::seed_seq
    return std::seed_seq(dev_words.begin(), dev_words.end());
}

/// @brief Returns a randomly seeded RNG
template<typename RNG>
RNG
randomly_seeded()
{
    // Some constants etc.
    using word_type = typename RNG::word_type;
    constexpr std::size_t n_words = RNG::word_count();
    constexpr std::size_t n_bits = RNG::bit_count();;

    // An appropriate std::seed_seq for this RNG.
    auto seq = seed_sequence(n_bits);

    // A seed array we will fill for this RNG.
    std::array<word_type, n_words> seed_array;

    // The generate method of a std::seed_seq is only guaranteed to produce 32-bit numbers.
    if constexpr (sizeof(word_type) <= 4) {
        // Great--the seq.generate method will happily fully fill our seed array completely
        seq.generate(seed_array.begin(), seed_array.end());
    }
    else {
        // 64-bit space: it will take two generate() calls per seed word in our generator
        std::array<std::uint32_t, 2 * n_words> tmp;
        seq.generate(tmp.begin(), tmp.end());
        for (std::size_t i = 0; i < n_words; ++i) {
            auto l = static_cast<word_type>(tmp[2 * i]) << 32;
            auto r = static_cast<word_type>(tmp[2 * i + 1]);
            seed_array[i] = l | r;
        }
    }

    // Construct the RNG
    RNG retval(seed_array.cbegin(), seed_array.cend());
    return retval;
}

int
main()
{
    // Create a couple of randomly seeded 64-bit generators
    auto rng64_1 = randomly_seeded<xso::rng64>();
    auto rng64_2 = randomly_seeded<xso::rng64>();

    // Create a couple of randomly seeded 32-bit generators
    auto rng32_1 = randomly_seeded<xso::rng32>();
    auto rng32_2 = randomly_seeded<xso::rng32>();

    utilities::pretty_print_thousands();

    std::cout << "64-bit calls:\n" << rng64_1() << '\n' << rng64_2() << '\n';
    std::cout << "32-bit calls:\n" << rng32_1() << '\n' << rng32_2() << '\n';

    return 0;
}