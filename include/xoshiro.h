#pragma once
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nessan.fitzmaurice@me.com>
// SPDX-License-Identifier: MIT

/// @file
/// An implementation of the xoshiro/xoroshiro family of pseudorandom number generators.
/// See the [Introduction](docs/pages/xoshiro.md) page for more details.

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <format>
#include <iterator>
#include <limits>
#include <ostream>
#include <random>
#include <ranges>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

// If the `gf2` library is available, we provide extra methods---primarily for analysing new RNG variants.
// For normal use, this is not required & the single-header xoshiro.h works fine without it.
#ifdef GF2
    #include <gf2/gf2.h>
#endif

namespace xso {

// --------------------------------------------------------------------------------------------------------------------
// A C++ concept that lets us distinguish standard distribution types from other types.
// --------------------------------------------------------------------------------------------------------------------

/// A C++ concept that lets us distinguish standard distribution types from other types.
///
/// We overload the `xso::generator::sample` method and use this concept to select one of those overloads.
///
/// # Note
/// Standard distributions all define a typename `param_type` which is not present in any of the container types.
template<typename Dist>
concept Distribution = requires { typename Dist::param_type; };

// --------------------------------------------------------------------------------------------------------------------
// Forward declarations of methods defined later in this file.
// --------------------------------------------------------------------------------------------------------------------
template<typename State>
constexpr auto jump_coefficients(std::size_t J, bool J_is_log2 = false);

template<std::unsigned_integral word_type, std::size_t N>
constexpr auto reduce(const std::array<word_type, N>& p, std::size_t J, bool J_is_log2 = false);

// --------------------------------------------------------------------------------------------------------------------
// The xoshiro/xoroshiro pseudorandom number generator class template.
// --------------------------------------------------------------------------------------------------------------------

/// A pseudorandom number generator combining a State and a Scrambler.
///
/// @tparam State     Stores the generator state, and has a `step` method to advance it.
/// @tparam Scrambler A functor to reduce the State to a single output word (a 32 or 64 bit unsigned).
///
/// # Note
/// Most users will not use this directly but instead reference one of the type aliased generators below.
template<typename State, typename Scrambler>
class generator {
public:
    /// @name Type definitions and class methods:
    /// @{

    /// The State used for by generator type -- one of the two state classes defined below.
    using state_type = State;

    /// The Scrambler used for by generator type -- one of the scrambler classes defined below.
    using scrambler_type = Scrambler;

    /// This generator type packs its state into words of this type (in practice, 32 or 64 bit unsigneds).
    using word_type = typename State::word_type;

    /// A convenience container type to hold the full state of this generator, jump polynomial coefficients, etc.
    using array_type = typename State::array_type;

    /// Class method that returns the number of words of state for this type of generator.
    ///
    /// # Example
    /// ```
    /// assert_eq(xso::rng::word_count(), 4);
    /// ```
    static constexpr std::size_t word_count() { return State::word_count(); }

    /// Class method that returns the number of bits of state for this type of generator.
    ///
    /// # Example
    /// ```
    /// assert_eq(xso::rng::bit_count(), 256);
    /// ```
    static constexpr std::size_t bit_count() { return State::bit_count(); }

    /// Class method that returns a name for this _type_ of generator --- combining State and Scrambler names.
    ///
    /// The returned string is for the generator type as a whole, combining both the State and Scrambler type names.
    /// There is no instance specific information in the returned string (i.e. no seed/state data).
    ///
    /// For example: `xoshiro<4x64,17,45> with star_star<5,7,1>` for an xoshiro generator with four 64-bit state words,
    /// using parameters 17 and 45 to step the state, combined with the "**" scrambler using parameters 5, 7 and 1.
    ///
    /// # Example
    /// ```
    /// assert_eq(xso::rng::type_string(), "xoshiro<4x64,17,45> with star_star<5,7,9,1>");
    /// ```
    static constexpr auto type_string() { return std::format("{} with {}", State::type_string(), Scrambler::type_string()); }

    /// @}
    /// @name Items required for the std::uniform_random_bit_generator concept:
    /// @{

    /// The unsigned integer type returned by the generator's `operator()()` method.
    ///
    /// # Note
    /// - This typename is required to satisfy the `std::uniform_random_bit_generator` concept.
    /// - For our generators this is always identical to the `word_type` used to store the state.
    using result_type = word_type;

    /// Returns the smallest value this generator can produce.
    ///
    /// # Note
    /// This method is required by the `std::uniform_random_bit_generator` concept.
    ///
    /// # Example
    /// ```
    /// assert_eq(xso::rng::min(), 0);
    /// ```
    static constexpr result_type min() noexcept { return 0; }

    /// Returns the largest value this generator can produce.
    ///
    /// # Note
    /// This method is required by the `std::uniform_random_bit_generator` concept.
    ///
    /// # Example
    /// ```
    /// assert_eq(xso::rng::max(), std::numeric_limits<typename xso::rng::result_type>::max());
    /// ```
    static constexpr result_type max() noexcept { return std::numeric_limits<result_type>::max(); }

    /// @}
    /// @name Constructors:
    /// @{

    /// The default constructor seeds the full underlying state randomly.
    ///
    /// This will produce a _high quality_ stream of random outputs that are different on each run.
    ///
    /// # Example
    /// ```
    /// xso::rng rng;
    /// assert(rng() != rng());  // Very, very unlucky if they are equal!
    /// ```
    constexpr generator() { seed(); }

    /// Construct a generator quickly but **not well** from a single unsigned integer value.
    ///
    /// Seeding from a single unsigned value is an easy way to get repeatable random streams.
    ///
    /// # Example
    /// ```
    /// xso::rng rng0{0x12345678};
    /// xso::rng rng1{0x12345678};
    /// assert_eq(rng0(), rng1());  // Same seed => same stream
    /// ```
    explicit constexpr generator(word_type s) { seed(s); }

    /// Construct and seed from an iteration of unsigned words which are all copied into the state.
    ///
    /// - The values in the iteration must be convertible to the generator's word_type.
    /// - The number of words provided must match the number of words in the generator's state.
    ///
    /// **Note:** The words shouldn't all be zeros as that is a fixed point for all xoshiro/xoroshiro generators.
    ///
    /// # Example
    /// ```
    /// xso::rng::array_type seed_words = {0x12345678, 0x9abcdef0, 0x13579bdf, 0x2468ace0};
    /// xso::rng rng0{seed_words.cbegin(), seed_words.cend()};
    /// xso::rng rng1{seed_words.cbegin(), seed_words.cend()};
    /// assert_eq(rng0(), rng1());                                      // Same seed => same stream
    /// xso::rng::array_type zero_words = {0, 0, 0, 0};
    /// xso::rng rng2{zero_words.cbegin(), zero_words.cend()};          // Bad: all zero seed!
    /// for (auto i = 0uz; i < 40; ++i) assert_eq(rng2(), 0);           // Will always produce zeros!
    /// ```
    template<std::input_iterator Iter>
        requires std::convertible_to<std::iter_value_t<Iter>, word_type>
    explicit constexpr generator(Iter b, Iter e) {
        seed(b, e);
    }

    /// @}
    /// @name Seeding methods:
    /// @{

    /// Seeds the full state to random starting values.
    ///
    /// This will produce a high quality stream of random outputs that are different on each run.
    ///
    /// # Example
    /// ```
    /// xso::rng rng0{0x12345678};
    /// xso::rng rng1{0x12345678};
    /// assert_eq(rng0(), rng1());          // Same seed => same stream
    /// assert_eq(rng0(), rng1());          // Same seed => same stream
    /// rng0.seed();                        // Now reseed randomly
    /// assert(rng0() != rng1());           // Very, very unlucky if they are equal!
    /// ```
    constexpr void seed() {
        // We will use std::random_device as the principal source of entropy.
        std::random_device dev;

        // Fill a full seed array with calls to dev() --  may need a couple of dev() calls to fill one word of state.
        array_type full_state;
        if constexpr (sizeof(word_type) <= sizeof(std::random_device::result_type)) {
            for (auto& word : full_state) word = static_cast<word_type>(dev());
        } else {
            for (auto& word : full_state) word = static_cast<word_type>(static_cast<uint64_t>(dev()) << 32 | dev());
        }

        // However, `std::random_device` may be poor so add data from a call to a high resolution clock for first word.
        using clock_type = std::chrono::high_resolution_clock;
        auto ticks = static_cast<std::uint64_t>(clock_type::now().time_since_epoch().count());

        // From call to call, time ticks only changes in the low order bits -- better scramble things a bit!
        ticks = murmur_scramble64(ticks);

        // Fold the scrambled ticks variable into the first seed word.
        full_state[0] ^= static_cast<word_type>(ticks);

        // Seed the state from our full "high quality" seed array.
        m_state.seed(full_state.cbegin(), full_state.cend());
    }

    /// Seeds the generator quickly but probably **not well** from a single unsigned integer value.
    ///
    /// Seeding from a single unsigned value is an easy way to get repeatable random streams.
    ///
    /// # Example
    /// ```
    /// xso::rng rng0, rng1;
    /// assert(rng0() != rng1());   // Very, very unlucky if they are equal!
    /// rng0.seed(0x12345678);
    /// rng1.seed(0x12345678);
    /// assert_eq(rng0(), rng1());  // Same seed => same stream
    /// ```
    constexpr void seed(word_type seed) {
        // Scramble the bits in the single seed we were given.
        auto sm64_state = murmur_scramble64(seed);

        // Use SplitMix64 to at least put some values in all the state words & seed the state.
        array_type full_state;
        for (auto& word : full_state) word = static_cast<word_type>(split_mix64(sm64_state));
        m_state.seed(full_state.cbegin(), full_state.cend());
    }

    /// Seeds the generator from an iteration of unsigned words which are all copied into the state.
    ///
    /// - The values in the iteration must be convertible to the generator's word_type.
    /// - The number of words provided must match the number of words in the generator's state.
    ///
    /// **Note:** The words shouldn't all be zeros as that is a fixed point for all xoshiro/xoroshiro generators.
    ///
    /// # Example
    /// ```
    /// xso::rng rng0, rng1;
    /// assert(rng0() != rng1());                                       // Very, very unlucky if they are equal!
    /// xso::rng::array_type seed_words = {0x12345678, 0x9abcdef0, 0x13579bdf, 0x2468ace0};
    /// rng0.seed(seed_words.cbegin(), seed_words.cend());
    /// rng1.seed(seed_words.cbegin(), seed_words.cend());
    /// assert_eq(rng0(), rng1());                                      // Same seed => same stream
    /// xso::rng::array_type zero_words = {0, 0, 0, 0};
    /// rng0.seed(zero_words.cbegin(), zero_words.cend());              // Bad: all zero seed!
    /// for (auto i = 0uz; i < 40; ++i) assert_eq(rng0(), 0);           // Will always produce zeros!
    /// ```
    template<std::input_iterator Src>
        requires std::convertible_to<std::iter_value_t<Src>, word_type>
    constexpr void seed(Src b, Src e) {
        m_state.seed(b, e);
    }

    /// @}
    /// @name Advance and output methods:
    /// @{

    /// Advances the state by one step.
    constexpr void step() { m_state.step(); }

    /// Returns the next random value from the generator which is a `result_type` unsigned integer.
    ///
    /// This scrambles the state to produce a single output, then advances the state to prepare for the next call.
    ///
    /// # Note
    /// This method signature is required by the `std::uniform_random_bit_generator` concept.
    constexpr result_type operator()() {
        result_type result = m_scrambler(m_state);
        step();
        return result;
    }

    /// @}
    /// @name State access methods:
    /// @{

    /// Returns the i'th state word.
    constexpr word_type operator[](std::size_t i) const { return m_state[i]; }

    /// Copies the whole state into the destination iterator `dst`
    ///
    /// - The iterator's value type must be convertible from the generator's `word_type`.
    /// - The iterator must be able to accept `word_count()` values.
    template<std::output_iterator<word_type> Dst>
    constexpr void get_state(Dst dst) const {
        m_state.get_state(dst);
    }

    /// @}
    /// @name Sampling methods:
    /// @{

    /// Sampling method that returns a single _integer_ value from a uniform distribution over `[a, b]`.
    ///
    /// **Note:** No error checking is done and the behaviour is undefined if `a > b`.
    ///
    /// # Example
    /// ```
    /// xso::rng rng;
    /// int b = 3, a  = -b;
    /// double mean = 0;
    /// std::size_t n_trials = 1'000'000;
    /// for (auto i = 0uz; i < n_trials; ++i) mean += rng.sample(a, b);
    /// mean /= static_cast<double>(n_trials);
    /// assert(mean > -0.1 && mean < 0.1);  // Should be close to 0.0
    /// ```
    template<std::integral T>
    constexpr T sample(T a, T b) {
        return std::uniform_int_distribution<T>{a, b}(*this);
    }

    /// Sampling method that returns a single _real_ value from a uniform distribution over `[a, b)`.
    ///
    /// **Note:** No error checking is done and the behaviour is undefined if `a > b`.
    ///
    /// # Example
    /// ```
    /// xso::rng rng;
    /// double b = 3, a  = -b;
    /// double mean = 0;
    /// std::size_t n_trials = 1'000'000;
    /// for (auto i = 0uz; i < n_trials; ++i) mean += rng.sample(a, b);
    /// mean /= static_cast<double>(n_trials);
    /// assert(mean > -0.1 && mean < 0.1);  // Should be close to 0.0
    /// ```
    template<std::floating_point T>
    constexpr T sample(T a, T b) {
        return std::uniform_real_distribution<T>{a, b}(*this);
    }

    /// Sampling method that returns a single index from a uniform distribution over `[0, len-1]`.
    ///
    /// This is a convenience method for sampling an index into an array or container of length `len`.
    /// It is equivalent to calling `sample(0, len-1)`.
    ///
    /// **Note:** No error checking is done and the behaviour is undefined if `len == 0`.
    ///
    /// Example
    /// ```
    /// xso::rng rng;
    /// std::vector<int> vec = {10, 20, 30, 40, 50};
    /// auto idx = rng.index(vec.size());
    /// assert(idx >= 0 && idx < vec.size());
    /// ```
    template<std::integral T>
    constexpr T index(T len) {
        return sample(T{0}, len - 1);
    }

    /// Sampling method that returns a single value from an iteration -- all elements are equally likely to be returned.
    ///
    /// **Note:** No error checking is done and the behaviour is undefined if `b > e`.
    ///
    /// # Example
    /// ```
    /// xso::rng rng;
    /// std::vector<std::size_t> vec = {0, 1, 2, 3, 4, 5};
    /// std::size_t n_trials = 1'000'000;
    /// std::array<std::size_t, 6> counts = {0};
    /// for (auto i = 0uz; i < n_trials; ++i) { ++counts[rng.sample(vec.cbegin(), vec.cend())]; }
    /// for (auto c : counts) {
    ///     double fraction = static_cast<double>(c) / static_cast<double>(n_trials);
    ///     assert(fraction > 0.16 && fraction < 0.18);  // Each element should be chosen about 1/6 of the time
    /// }
    /// ```
    template<std::input_iterator T>
    constexpr auto sample(T b, T e) {
        // Edge case?
        auto len = std::distance(b, e);
        if (len < 2) return *b;

        // Pick an index inside the iteration at random & return the corresponding value.
        auto i = index(len);
        std::advance(b, i);
        return *b;
    }

    /// Sampling method that returns a single value from a container -- all elements are equally likely to be returned.
    ///
    /// The "container" can be any type that supports `std::ranges::cbegin()` and `std::ranges::cend()`.
    ///
    /// **Note:** No error checking is done and the behaviour is undefined if the container is empty.
    ///
    /// # Example
    /// ```
    /// xso::rng rng;
    /// std::vector<int> vec = {0, 1, 2, 3, 4, 5};
    /// std::size_t n_trials = 1'000'000;
    /// std::array<std::size_t, 6> counts = {0};
    /// for (auto i = 0uz; i < n_trials; ++i) { ++counts[static_cast<std::size_t>(rng.sample(vec))]; }
    /// for (auto c : counts) {
    ///     double fraction = static_cast<double>(c) / static_cast<double>(n_trials);
    ///     assert(fraction > 0.16 && fraction < 0.18);  // Each element should be chosen about 1/6 of the time
    /// }
    /// ```
    template<typename Container>
        requires std::ranges::input_range<const Container> && std::ranges::common_range<const Container>
    constexpr auto sample(const Container& container) {
        return sample(std::ranges::cbegin(container), std::ranges::cend(container));
    }

    /// Sampling method that picks `n` elements _without replacement_ from an iteration `[b, e)` and puts them in `dst`.
    ///
    /// Selects `n` elements at random from the iteration defined by `[b, e)` and copies them to the destination
    /// iterator `dst`. This is done _without replacement_ so each element can only be selected once.
    ///
    /// The destination iterator `dst` must be able to accept at least `n` elements of the type stored in the iteration.
    /// The output elements in `dst` are in the order they were selected, which is random.
    ///
    /// **Note:** No error checking is done and the behaviour is undefined if `e < b`.
    ///
    /// Example
    /// ```
    /// xso::rng rng;
    /// std::vector<int> vec = {10, 20, 30, 40, 50};
    /// std::vector<int> samples;
    /// rng.sample(vec.cbegin(), vec.cend(), std::back_inserter(samples), 3);
    /// assert_eq(samples.size(), 3);
    /// for (auto v : samples) assert(std::find(vec.cbegin(), vec.cend(), v) != vec.cend());
    /// auto sorted = samples;
    /// std::sort(sorted.begin(), sorted.end());
    /// assert(std::adjacent_find(sorted.cbegin(), sorted.cend()) == sorted.cend());
    /// ```
    template<std::input_iterator Src, std::output_iterator<std::iter_value_t<Src>> Dst>
    constexpr Dst sample(Src b, Src e, Dst dst, std::size_t n) {
        return std::sample(b, e, dst, n, *this);
    }

    /// Sampling method that picks `n` elements without replacement from a container and puts them in `dst`.
    ///
    /// Selects `n` elements at random from the `src` container and copies them to the destination iterator `dst`.
    /// This is done _without replacement_ so each element can only be selected once.
    ///
    /// The "container" can be any type that supports `std::ranges::cbegin()` and `std::ranges::cend()`.
    ///
    /// The destination iterator `dst` must be able to accept at least `n` elements of the type stored in the iteration.
    /// The output elements in `dst` are in the order they were selected, which is random.
    ///
    /// **Note:** No error checking is done and the behaviour is undefined if the container is empty.
    ///
    /// Example
    /// ```
    /// xso::rng rng;
    /// std::vector<int> vec = {10, 20, 30, 40, 50};
    /// std::vector<int> samples;
    /// rng.sample(vec, std::back_inserter(samples), 3);
    /// assert_eq(samples.size(), 3);
    /// for (auto v : samples) assert(std::find(vec.cbegin(), vec.cend(), v) != vec.cend());
    /// auto sorted = samples;
    /// std::sort(sorted.begin(), sorted.end());
    /// assert(std::adjacent_find(sorted.cbegin(), sorted.cend()) == sorted.cend());
    /// ```
    template<typename Src, std::output_iterator<std::ranges::range_value_t<const Src>> Dst>
        requires std::ranges::input_range<const Src> && std::ranges::common_range<const Src>
    constexpr auto sample(const Src& src, Dst dst, std::size_t n) {
        return sample(std::ranges::cbegin(src), std::ranges::cend(src), dst, n);
    }

    /// Sampling method that returns a single random variate drawn from a distribution.
    ///
    /// The distribution must satisfy the `xso::Distribution` concept, which is true of all standard distributions.
    ///
    /// # Example
    /// ```
    /// xso::rng rng;
    /// std::normal_distribution<double> dist{0.0, 1.0};
    /// std::size_t n_samples = 1'000'000;
    /// double sum = 0.0;
    /// for (auto i = 0uz; i < n_samples; ++i) sum += rng.sample(dist);
    /// double mean = sum / static_cast<double>(n_samples);
    /// assert(mean > -0.01 && mean < 0.01);  // Should be close to 0.0
    /// ```
    constexpr auto sample(Distribution auto& dist) { return dist(*this); }

    /// Sampling method that takes `n` samples from a distribution and puts them into a destination iterator.
    ///
    /// # Note
    /// - The distribution must satisfy the `xso::Distribution` concept, which is true of all standard distributions.
    /// - The destination iterator `dst` must be able to accept at least `n` elements of the type produced by the
    /// distribution.
    ///
    /// # Example
    /// ```
    /// xso::rng rng;
    /// std::normal_distribution<double> dist{0.0, 1.0};
    /// std::size_t n_samples = 1'000'000;
    /// std::vector<double> samples;
    /// rng.sample(dist, std::back_inserter(samples), n_samples);
    /// double sum = 0.0;
    /// for (auto v : samples) sum += v;
    /// double mean = sum / static_cast<double>(n_samples);
    /// assert(mean > -0.01 && mean < 0.01);  //
    /// ```
    template<typename Dst>
    constexpr Dst sample(Distribution auto& dist, Dst dst, std::size_t n) {
        while (n-- != 0) *dst++ = dist(*this);
        return dst;
    }

    /// Sampling method that rolls a dice with an arbitrary number of sides (defaults to the usual 6).
    ///
    /// # Example
    /// ```
    /// xso::rng rng;
    /// std::size_t n_rolls = 1'000'000;
    /// std::array<std::size_t, 6> counts = {0};
    /// for (auto i = 0uz; i < n_rolls; ++i) { ++counts[rng.roll() - 1]; }
    /// for (auto c : counts) {
    ///     double fraction = static_cast<double>(c) / static_cast<double>(n_rolls);
    ///     assert(fraction > 0.16 && fraction < 0.18);  // Each side should come up about 1/6 of the time
    /// }
    /// ```
    constexpr std::size_t roll(std::size_t n_sides = 6) { return sample(1uz, n_sides); }

    /// Sampling method that flips a coin, where the probability of getting `true` is `p` (defaults to 0.5).
    ///
    /// # Example
    /// ```
    /// xso::rng rng;
    /// std::size_t true_count = 0;
    /// std::size_t n_trials = 1'000'000;
    /// for (auto i = 0uz; i < n_trials; ++i) if (rng.flip()) ++true_count;
    /// double true_fraction = static_cast<double>(true_count) / static_cast<double>(n_trials);
    /// assert(true_fraction > 0.49 && true_fraction < 0.51);  // Should be close to 0.5
    /// ```
    constexpr bool flip(double p = 0.5) { return std::bernoulli_distribution{p}(*this); }

    /// @}
    /// @name Shuffling methods:
    /// @{

    /// This method shuffles the elements in an iteration.
    ///
    /// **Note:** No error checking is done and the behaviour is undefined if `e < b`.
    ///
    /// # Example
    /// ```
    /// xso::rng rng;
    /// std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    /// rng.shuffle(vec.begin(), vec.end());
    /// assert(vec != std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));  // Very, very unlucky if they are equal!
    /// std::sort(vec.begin(), vec.end());
    /// assert(vec == std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));  // Should be back in order now
    /// ```
    template<std::random_access_iterator Iter>
    constexpr void shuffle(Iter b, Iter e) {
        std::shuffle(b, e, *this);
    }

    /// This method shuffles all the elements of a container.
    ///
    /// **Note:** No error checking is done and the behaviour is undefined if the container is empty.
    ///
    /// # Example
    /// ```
    /// xso::rng rng;
    /// std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    /// rng.shuffle(vec);
    /// assert(vec != std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));  // Very, very unlucky if they are equal!
    /// std::sort(vec.begin(), vec.end());
    /// assert(vec == std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));  // Should be back in order now
    /// ```
    template<typename Container>
        requires std::ranges::random_access_range<Container> && std::ranges::common_range<Container>
    constexpr void shuffle(Container& container) {
        return shuffle(std::begin(container), std::end(container));
    }

    /// @}
    /// @name Discard method:
    /// @{

    /// Discards the next `z` iterations in the random number sequence.
    ///
    /// This is equivalent to calling `step()` `z` times and is the typical way used to discard random values in the
    /// standard library. It is here for compatibility with that interface but is basically rubbish.
    ///
    /// **Note:** We can do much better for large `z` by using one of the `jump` methods.
    void discard(std::uint64_t z) {
        for (std::uint64_t i = 0; i < z; ++i) step();
    }

    /// @}
    /// @name Jump methods:
    /// @{

    /// Jumps the generator's state forward by `J` steps where `J` is either `n` or `2^n` to accommodate huge jumps that
    /// do not fit into the usual integer range.
    ///
    /// This method also returns the jump polynomial coefficients used to perform the jump. These coefficients can be
    /// reused to efficiently and repeatedly jump by the same number of steps using the alternate `jump()` method below.
    ///
    /// # Example
    /// ```
    /// xso::rng r0;
    /// xso::rng r1 = r0;
    /// auto J = 1'000'000uz;
    /// auto jump_coeffs = r0.jump(J);
    /// r1.discard(J);
    /// for (auto i = 0uz; i < xso::rng::word_count(); ++i) assert_eq(r0[i], r1[i]);
    /// assert_eq(r0(), r1());
    /// r0.jump(jump_coeffs);   // Jump again by same amount but more efficiently
    /// r1.discard(J);
    /// for (auto i = 0uz; i < xso::rng::word_count(); ++i) assert_eq(r0[i], r1[i]);
    /// assert_eq(r0(), r1());
    /// ```
    constexpr auto jump(std::size_t n, bool n_is_log2 = false) {
        // Get the jump polynomial coefficients for this jump
        auto jump_coeffs = xso::jump_coefficients<state_type>(n, n_is_log2);

        // Perform the jump using those coefficients
        jump(jump_coeffs);

        // Return the jump polynomial coefficients to the caller for possible reuse later.
        return jump_coeffs;
    }

    /// Efficiently jumps the generator's state forward by `J` steps where `J` can be huge (e.g. 2^100)
    ///
    /// This jump method must be passed _pre-computed_ jump polynomial coefficients which can be calculated using
    /// the `xso::jump_coefficients<State>()` free function. This is the preferred way to jump the state forward by a
    /// large number of steps when you need to do it often as the jump polynomial can be computed once and reused many
    /// times.
    ///
    /// # Example
    /// ```
    /// xso::rng r0;
    /// xso::rng r1 = r0;
    /// auto J = 1'000'000uz;
    /// auto jump_coefficients = xso::jump_coefficients<xso::rng::state_type>(J);
    /// r0.jump(jump_coefficients);
    /// r1.discard(J);
    /// for (auto i = 0uz; i < xso::rng::word_count(); ++i) assert_eq(r0[i], r1[i]);
    /// assert_eq(r0(), r1());
    /// ```
    constexpr void jump(const array_type& jump_coefficients) {

        // Constant
        constexpr std::size_t bits_per_word = std::numeric_limits<word_type>::digits;

        // lambda: `is_set(word, b)` returns true if bit `b` in `word` is set.
        auto is_set = [=](const word_type word, std::size_t b) -> bool {
            return word & static_cast<word_type>(word_type{1} << b);
        };

        // If the current state is `s` and the state's transition matrix is T then the jump we want is s <- (T^n).s.
        // Cayley-Hamilton is invoked to equate T^n with a small polynomial sum r(T) where r(x) = x^n mod c(x)
        // and c(x) is the characteristic polynomial for T. Thus (T^n).s == r(T).s which is doable.
        //
        // The `jump_coefficients` argument is assumed to have the coefficients of r(x) stored compactly in words.
        // The next loop computes the sum r(T).s = r_0.s + r_1.s^1 + ... + r_{n-1} s^{n-1}.
        // We compute s^{i+1} = T.s^i using the step() method (so s <- T.s <- (T^2).s by iteratively stepping s
        // forward).
        array_type sum;
        sum.fill(0);
        for (auto i = 0uz; i < word_count(); ++i) {

            // Iterate over the bits in each coefficient word -- the bits are the coefficients of r(x).
            word_type r_word = jump_coefficients[i];
            for (auto b = 0uz; b < bits_per_word; ++b) {
                if (is_set(r_word, b)) {
                    // This coefficient in r(x) is one, so contributes to the sum ...
                    for (auto w = 0uz; w < word_count(); ++w) sum[w] ^= m_state[w];
                }
                // Compute the next state s^{i+1} by calling the step() method (same as s <- T.s).
                m_state.step();
            }
        }

        // Store the computed jumped state back as our current state so we are ready to proceed from there ...
        m_state.seed(sum.cbegin(), sum.cend());
    }

    /// Class method that fills a destination iterator with the state's _precomputed_ characteristic polynomial
    /// coefficients.
    ///
    /// For the purpose of analysis, we note that the state advance algorithm can be represented by the linear
    /// transformation $\mathbf{s} \leftarrow T \cdot \mathbf{s},$  where $\mathbf{s}$ is the state vector, and $T$ is
    /// an $n \times n$ full-rank matrix over GF(2).
    ///
    /// The characteristic polynomial $c(x)$ for the transition matrix $T$ can be written as $c(x) = x^n + p(x)$
    /// where $p(x) = p_0 + p_1 x + ... + p_{n-1} x^{n-1}$.
    ///
    /// The polynomial $p(x)$ can be represented by its $n$ bit coefficients. Those coefficients are precomputed and
    /// stored in word form for efficiency.
    ///
    /// **Note:** The destination iterator must be able to accept `word_count()` values of type `word_type`.
    ///
    /// # Panics
    /// In practice, we have precomputed the characteristic polynomial for just a few xoshiro/xoroshiro's with specific
    /// parameters. If the `State` has no precomputed characteristic coefficients this method will throw an exception.
    template<std::output_iterator<word_type> Dst>
    static constexpr void characteristic_coefficients(Dst dst) {
        State::characteristic_coefficients(dst);
    }

    /// Class method that returns the state's _precomputed_ characteristic polynomial coefficients packed into an array
    /// of words.
    ///
    /// For the purpose of analysis, we note that the state advance algorithm can be represented by the linear
    /// transformation $\mathbf{s} \leftarrow T \cdot \mathbf{s},$  where $\mathbf{s}$ is the state vector, and $T$ is
    /// an $n \times n$ full-rank matrix over GF(2).
    ///
    /// The characteristic polynomial $c(x)$ for the transition matrix $T$ can be written as $c(x) = x^n + p(x)$
    /// where $p(x) = p_0 + p_1 x + ... + p_{n-1} x^{n-1}$.
    ///
    /// The polynomial $p(x)$ can be represented by its $n$ bit coefficients. Those coefficients are precomputed and
    /// stored in word form for efficiency.
    ///
    /// # Panics
    /// In practice, we have precomputed the characteristic polynomial for just a few xoshiro/xoroshiro's with specific
    /// parameters. If the `State` has no precomputed characteristic coefficients this method will throw an exception.
    static constexpr auto characteristic_coefficients() {
        array_type result;
        State::characteristic_coefficients(result.begin());
        return result;
    }

#ifdef GF2

    /// @}
    /// @name Extra method that uses the optional `gf2` library.
    /// @{

    /// Jumps a state/generator ahead in its random number stream by `J` steps.
    ///
    /// This method must be passed a precomputed jump polynomial as a bit-polynomial from the `jump_polynomial` method.
    void jump(gf2::BitPolynomial<word_type> const& jump_poly) {
        array_type sum;

        // Computing [r_0 + r_1 T + ... + r_{m-1} T^{m-1}].s where s is the current state and r is the jump polynomial.
        // T is the state's transition matrix so we can compute s^{i+1} = T.s^i using the step() method.
        sum.fill(0);
        for (auto i = 0uz; i < jump_poly.size(); ++i) {
            if (jump_poly[i])
                for (std::size_t w = 0; w < word_count(); ++w) sum[w] ^= m_state[w];
            m_state.step();
        }

        // Perform the computed jump by reseeding the state from the computed sum ...
        m_state.seed(sum.cbegin(), sum.cend());
    }
#endif
    /// @}

private:
    State     m_state;
    Scrambler m_scrambler;

    // Class method that is an implementation of the SplitMix64 random number generator -- a simple generator with 64
    // bits of state.
    //
    // Used here to help seed our more complex generators from a single 64-bit seed value.
    static constexpr std::uint64_t split_mix64(std::uint64_t& state) {
        std::uint64_t z = (state += 0x9e3779b97f4a7c15);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
        z = (z ^ (z >> 31));
        return z;
    };

    // Class method that returns a 64-bit word that is a scrambled version of the input 64-bit value.
    //
    // This is based on the MurmurHash3 finalizer function.
    static constexpr std::uint64_t murmur_scramble64(std::uint64_t x) {
        x ^= x >> 33;
        x *= 0xff51afd7ed558ccdL;
        x ^= x >> 33;
        x *= 0xc4ceb9fe1a85ec53L;
        x ^= x >> 33;
        return x;
    }

    // Class method that returns a 32-bit word that is a scrambled version of the input 32-bit value.
    //
    // This is based on the MurmurHash3 mix function.
    static constexpr std::uint32_t murmur_scramble32(std::uint32_t x) {
        x *= 0xcc9e2d51;
        x = (x << 15) | (x >> 17);
        x *= 0x1b873593;
        return x;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The Xoshiro state:
// --------------------------------------------------------------------------------------------------------------------

/// The state for the Xoshiro family of pseudorandom number generators.
///
/// - The state consists of `N` words of some unsigned integer type `T`.
/// - It is advanced using the Xoshiro algorithm with parameters `A` and `B`.
template<std::size_t N, std::unsigned_integral T, std::uint8_t A, std::uint8_t B>
class xoshiro {
public:
    /// The state is stored as an array of `N` words of type `T`.
    using word_type = T;

    /// A convenience container type to hold the full state of this generator, jump polynomial coefficients, etc.
    using array_type = std::array<T, N>;

    /// Class method that returns the number of words in the underlying state.
    static constexpr std::size_t word_count() { return N; }

    /// Class method that returns the number of bits of state.
    static constexpr std::size_t bit_count() { return N * std::numeric_limits<T>::digits; }

    /// Class method that returns a name for this _type_ of state.
    static constexpr auto type_string() {
        return std::format("xoshiro<{}x{},{},{}>", N, std::numeric_limits<T>::digits, A, B);
    }

    /// Returns the i'th state word.
    constexpr T operator[](std::size_t i) const { return m_state[i]; }

    /// Copies the whole state into the destination iterator `dst`
    ///
    /// - The iterator's value type must be convertible from the generator's `word_type`.
    /// - The iterator must be able to accept `word_count()` values.
    template<std::output_iterator<word_type> Dst>
    constexpr void get_state(Dst dst) const {
        std::copy(m_state.cbegin(), m_state.cend(), dst);
    }

    /// Sets the state from an iteration of words.
    ///
    /// - The iterator's value type must be convertible to the generator's `word_type`.
    /// - The iteration should provide `word_count()` values.
    /// - The iteration shouldn't provide all zeros as that is fixed point for all these generators.
    template<std::input_iterator Src>
        requires std::convertible_to<std::iter_value_t<Src>, word_type>
    constexpr void seed(Src b, Src e) {
        std::copy(b, e, m_state.begin());
    }

    /// Advance the state by one step.
    ///
    /// # Panics
    /// There is no discernible pattern to the way `xoshiro` works as the number of words of state increases.
    /// The `step()` method for each `N` has to be hard coded -- this contrasts to `xoroshiro` state below.
    /// We panic at compile time if we don't have a formula that works to advance the state.
    constexpr void step() {
        if constexpr (N == 4) {
            auto tmp = m_state[1] << A;
            m_state[2] ^= m_state[0];
            m_state[3] ^= m_state[1];
            m_state[1] ^= m_state[2];
            m_state[0] ^= m_state[3];
            m_state[2] ^= tmp;
            m_state[3] = std::rotl(m_state[3], B);
        } else if constexpr (N == 8) {
            auto tmp = m_state[1] << A;
            m_state[2] ^= m_state[0];
            m_state[5] ^= m_state[1];
            m_state[1] ^= m_state[2];
            m_state[7] ^= m_state[3];
            m_state[3] ^= m_state[4];
            m_state[4] ^= m_state[5];
            m_state[0] ^= m_state[6];
            m_state[6] ^= m_state[7];
            m_state[6] ^= tmp;
            m_state[7] = std::rotl(m_state[7], B);
        } else {
            // Get a useful'ish error message by pumping a deliberately false condition into static_assert(...).
            static_assert(N < 0, "No xoshiro step() implementation for this number of words of state!");
        }
    }

    /// Class method that fills a destination iterator with our _precomputed_ characteristic polynomial coefficients.
    ///
    /// If the characteristic polynomial is $c(x) = x^n + p(x)$ then we fill the destination with the coefficients of
    /// $p(x)$ packed into words.
    ///
    /// # Panics
    /// In practice, we have precomputed the characteristic polynomial for just a few xoshiro/xoroshiro's with specific
    /// parameters. If this `State` has no precomputed characteristic coefficients this method will throw an exception.
    template<std::output_iterator<word_type> Dst>
    static constexpr void characteristic_coefficients(Dst dst) {
        // In practice we have precomputed the p(x) polynomial for just a few xoshiro's with specific parameters.
        if constexpr (std::is_same_v<T, uint32_t> && N == 4 && A == 9 && B == 11) {
            std::array<T, N> p = {0xde18fc01, 0x1b489db6, 0x6254b1, 0xfc65a2};
            std::copy(p.cbegin(), p.cend(), dst);
        } else if constexpr (std::is_same_v<T, uint64_t> && N == 4 && A == 17 && B == 45) {
            std::array<T, N> p = {0x9d116f2bb0f0f001, 0x280002bcefd1a5e, 0x4b4edcf26259f85, 0x3c03c3f3ecb19};
            std::copy(p.cbegin(), p.cend(), dst);
        } else if constexpr (std::is_same_v<T, uint64_t> && N == 8 && A == 11 && B == 21) {
            std::array<T, N> p = {0xcf3cff0c00000001, 0x7fdc78d886f00c63, 0xf05e63fca6d7b781, 0x7a67058e7bbab6f0,
                                  0xf11eef832e32518f, 0x51ba7c47edc758ad, 0x8f2d27268ce4b20b, 0x500055d8b77f};
            std::copy(p.cbegin(), p.cend(), dst);
        } else {
            throw std::invalid_argument("xoshiro characteristic polynomial not pre-computed for given parameters!");
        }
    }

    /// Class method that returns the state's _precomputed_ characteristic polynomial coefficients packed into an array
    /// of words.
    ///
    /// If the characteristic polynomial is $c(x) = x^n + p(x)$ then we return an array with the coefficients of
    /// $p(x)$ packed into words.
    ///
    /// # Panics
    /// In practice, we have precomputed the characteristic polynomial for just a few xoshiro/xoroshiro's with specific
    /// parameters. If this `State` has no precomputed characteristic coefficients this method will throw an exception.
    static constexpr auto characteristic_coefficients() {
        array_type result;
        characteristic_coefficients(result.begin());
        return result;
    }

private:
    std::array<T, N> m_state = {1};
};

// --------------------------------------------------------------------------------------------------------------------
// The Xoroshiro State:
// --------------------------------------------------------------------------------------------------------------------

/// The state for the Xoroshiro family of pseudorandom number generators.
///
/// - The state consists of `N` words of some unsigned integer type `T`. <br>
/// - It is advanced using the Xoshiro algorithm with parameters `A`, `B`, and `C`.
template<std::size_t N, std::unsigned_integral T, std::uint8_t A, std::uint8_t B, std::uint8_t C>
class xoroshiro {
public:
    /// The type of the words of state
    using word_type = T;

    /// A convenience container type to hold the full state of this generator, jump polynomial coefficients, etc.
    using array_type = std::array<T, N>;

    /// Class method that returns the number of words in the underlying state.
    static constexpr std::size_t word_count() { return N; }

    /// Class method that returns the number of bits of state.
    static constexpr std::size_t bit_count() { return N * std::numeric_limits<T>::digits; }

    /// Class method that returns a name for this state.
    static constexpr auto type_string() {
        return std::format("xoroshiro<{}x{},{},{},{}>", N, std::numeric_limits<T>::digits, A, B, C);
    }

    /// Returns the i'th state word.
    ///
    /// NOTE: For larger values of N we store the state as a ring buffer so the indexing is a bit more complex.
    constexpr T operator[](std::size_t i) const { return m_state[(i + m_final + 1) % N]; }

    /// Copies the whole state into the destination iterator `dst`
    ///
    /// - The iterator's value type must be convertible from the generator's `word_type`.
    /// - The iterator must be able to accept `word_count()` values.
    ///
    /// NOTE: For larger values of N we store the state as a ring buffer so we need to untangle it here.
    template<std::output_iterator<word_type> Dst>
    constexpr void get_state(Dst dst) const {
        for (auto i = 0uz; i < N; ++i, ++dst) *dst = operator[](i);
    }

    /// Sets the state from an iteration of words.
    ///
    /// - The iterator's value type must be convertible to the generator's `word_type`.
    /// - The iteration should provide `word_count()` values.
    /// - The iteration shouldn't provide all zeros as that is fixed point for all these generators.
    template<std::input_iterator Src>
        requires std::convertible_to<std::iter_value_t<Src>, word_type>
    constexpr void seed(Src b, Src e) {
        std::copy(b, e, m_state.begin());

        // Reset the ring buffer final index
        m_final = N - 1;
    }

    /// Advance the state by one step.
    constexpr void step() {
        // Depending on the size of `N` we either do an explicit or implicit array shuffle of the state array.
        if constexpr (N == 2)
            simple_step();
        else
            clever_step();
    }

    /// Class method that fills a destination iterator with our _precomputed_ characteristic polynomial coefficients.
    ///
    /// If the characteristic polynomial is $c(x) = x^n + p(x)$ then we fill the destination with the coefficients of
    /// $p(x)$ packed into words.
    ///
    /// # Panics
    /// In practice, we have precomputed the characteristic polynomial for just a few xoshiro/xoroshiro's with specific
    /// parameters. If this `State` has no precomputed characteristic coefficients this method will throw an exception.
    template<std::output_iterator<word_type> Dst>
    static constexpr void characteristic_coefficients(Dst dst) {
        if constexpr (std::is_same_v<T, uint32_t> && N == 2 && A == 26 && B == 9 && C == 13) {
            std::array<T, N> p = {0x6e2286c1, 0x53be9da};
            std::copy(p.cbegin(), p.cend(), dst);
        } else if constexpr (std::is_same_v<T, uint64_t> && N == 2 && A == 24 && B == 16 && C == 37) {
            std::array<T, N> p = {0x95b8f76579aa001, 0x8828e513b43d5};
            std::copy(p.cbegin(), p.cend(), dst);
        } else if constexpr (std::is_same_v<T, uint64_t> && N == 2 && A == 49 && B == 21 && C == 28) {
            std::array<T, N> p = {0x8dae70779760b081, 0x31bcf2f855d6e5};
            std::copy(p.cbegin(), p.cend(), dst);
        } else if constexpr (std::is_same_v<T, uint64_t> && N == 16 && A == 25 && B == 27 && C == 36) {
            std::array<T, N> p = {0x5cfeb8cc48ddb211, 0xb73e379d035a06dd, 0x17d5100a20a0350e, 0x7550223f68f98cac,
                                  0x29d373b5c5ed3459, 0x3689b412ef70de48, 0xa1d3b6ee079a7cc6, 0x9bf0b669abd100f8,
                                  0x955c84e105f60997, 0x6ca140c61889cddd, 0xabaf68c5fc3a0e4a, 0xa46134526b83adc5,
                                  0x710704d05683d63,  0x580d080b44b606a2, 0x8040a0580158a1,   0x800081};
            std::copy(p.cbegin(), p.cend(), dst);
        } else {
            throw std::invalid_argument("xoroshiro characteristic polynomial not pre-computed for given parameters!");
        }
    }

    /// Class method that returns the state's _precomputed_ characteristic polynomial coefficients packed into an array
    /// of words.
    ///
    /// If the characteristic polynomial is $c(x) = x^n + p(x)$ then we return an array with the coefficients of
    /// $p(x)$ packed into words.
    ///
    /// # Panics
    /// In practice, we have precomputed the characteristic polynomial for just a few xoshiro/xoroshiro's with specific
    /// parameters. If this `State` has no precomputed characteristic coefficients this method will throw an exception.
    static constexpr auto characteristic_coefficients() {
        array_type result;
        characteristic_coefficients(result.begin());
        return result;
    }

private:
    std::array<T, N> m_state = {1};   // The state is an array of words -- should never be all zeros!
    std::size_t      m_final = N - 1; // Current location of the final word of state.

    /// Step the state forward using a straight-forward move all the state words approach.
    /// @note  This is an alternative to the clever_step() and is used for small values of @c N.
    constexpr void simple_step() {
        // Capture the current values in the first and final words of state
        T s0 = m_state[0];
        T s1 = m_state[N - 1];

        // Shift most of the words of state down one slot
        // Note: It could help to unroll this loop at least once for larger N but the shuffle indices method is
        // better
        for (auto i = 0uz; i < N - 2; ++i) m_state[i] = m_state[i + 1];

        // Update the first and final words of state
        s1 ^= s0;
        m_state[N - 2] = std::rotl(s0, A) ^ (s1 << B) ^ s1;
        m_state[N - 1] = std::rotl(s1, C);
    }

    /// Step the state forward where we shuffle array indices instead of the state words.
    /// @note  This is an alternative to the simple_step() and is used for larger values of @c N.
    constexpr void clever_step() {
        // Which indices point to the current final & first words of state
        std::size_t i_final = m_final;
        std::size_t i_first = (m_final + 1) % N;

        // Capture the current values in the final & first words of state
        T s_final = m_state[i_final];
        T s_first = m_state[i_first];

        // Update the values for the final & first words of state
        s_final ^= s_first;
        m_state[i_final] = std::rotl(s_first, A) ^ (s_final << B) ^ s_final;
        m_state[i_first] = std::rotl(s_final, C);

        // Step the index of the final word of state -- this shuffles the state array down a slot.
        m_final = i_first;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The Scramblers:
//
// A Scrambler is a functor that is passed a State and returns a single unsigned output word.
// --------------------------------------------------------------------------------------------------------------------

/// The `star` scrambler is passed a `state` array and will return `S * state[w]`  where:
///
/// @tparam S  is a constant multiplier which should be _odd_ to ensure good mixing of bits.
/// @tparam w  is the  index of the word in the state array to use.
///
/// For this scrambler, any choice of `w` will do as all the words are getting updated by the state advance algorithm.
/// Typically, we use multipliers that are `2^s + 1` for some `s` as those are odd and compilers will optimize the
/// multiplication to a shift and add.
template<auto S, std::size_t w = 0>
struct star {
    constexpr auto operator()(const auto& state) const { return S * state[w]; }

    static constexpr auto type_string() { return std::format("star<{:x},{}>", S, w); }
};

/// The `star_star` scrambler is passed a `state` array an returns `T * rotl(S * state[w], R)` where:
///
/// @tparam S  is a constant multiplier that should be odd.
/// @tparam R  is a rotation amount.
/// @tparam T  is a constant multiplier applied after the rotation.
/// @tparam w  is the index of the word in the state array to use.
///
/// For this scrambler, any choice of `w` will do as all the words are getting updated by the state advance algorithm.
/// Typically, we use multipliers that are `2^s + 1` for some `s` as those are odd and compilers will optimize the
/// multiplication to a shift and add.
template<auto S, auto R, auto T, std::size_t w = 0>
struct star_star {
    constexpr auto operator()(const auto& state) const { return T * std::rotl(S * state[w], R); }

    static constexpr auto type_string() { return std::format("star_star<{:x},{},{:x},{}>", S, R, T, w); }
};

/// The `plus` scrambler is passed a `state` array an returns `state[w0] + state[w1]`.
///
/// @tparam w0 The index of the first word in the state array to use.
/// @tparam w1 The index of the second word in the state array to use.
///
/// For this scrambler, any choice of `w0` and `w1` will do as all the words are getting updated by the state advance
/// algorithm.
template<std::size_t w0 = 0, std::size_t w1 = 1>
struct plus {
    constexpr auto operator()(const auto& state) const { return state[w0] + state[w1]; }

    static constexpr auto type_string() { return std::format("plus<{},{}>", w0, w1); }
};

/// The `plus_plus` scrambler is passed a `state` array an returns `rotl(state[w0] + state[w1], R) + state[w0]`.
///
/// @tparam w0 The index of the first word in the state array to use.
/// @tparam w1 The index of the second word in the state array to use.
/// @tparam R  A rotation amount.
template<auto R, std::size_t w0, std::size_t w1>
struct plus_plus {
    constexpr auto operator()(const auto& state) const { return std::rotl(state[w0] + state[w1], R) + state[w0]; }

    static constexpr auto type_string() { return std::format("plus_plus<{},{},{}>", R, w0, w1); }
};

// --------------------------------------------------------------------------------------------------------------------
// Type aliases for all 17 generators discussed in the Black & Vigna paper
// --------------------------------------------------------------------------------------------------------------------

// clang-format off
// The analysed versions of the xoshiro engine with specific choices for A & B:
using xoshiro_4x32              = xoshiro<4, uint32_t, 9,  11>;
using xoshiro_4x64              = xoshiro<4, uint64_t, 17, 45>;
using xoshiro_8x64              = xoshiro<8, uint64_t, 11, 21>;

// The analysed versions of the xoshiro engine with specific choices for A, B & C:
using xoroshiro_2x32            = xoroshiro<2,  uint32_t, 26, 9,  13>;
using xoroshiro_2x64            = xoroshiro<2,  uint64_t, 24, 16, 37>;
using xoroshiro_2x64b           = xoroshiro<2,  uint64_t, 49, 21, 28>;  // Alternative for 2x64 case
using xoroshiro_16x64           = xoroshiro<16, uint64_t, 25, 27, 36>;

// The analyzed versions of the xoshiro generators:
using xoshiro_4x32_plus         = generator<xoshiro_4x32, plus<0, 3>>;
using xoshiro_4x32_plus_plus    = generator<xoshiro_4x32, plus_plus<7, 0, 3>>;
using xoshiro_4x32_star_star    = generator<xoshiro_4x32, star_star<5, 7, 9, 1>>;
using xoshiro_4x64_plus         = generator<xoshiro_4x64, plus<0, 3>>;
using xoshiro_4x64_plus_plus    = generator<xoshiro_4x64, plus_plus<23, 0, 3>>;
using xoshiro_4x64_star_star    = generator<xoshiro_4x64, star_star<5, 7, 9, 1>>;
using xoshiro_8x64_plus         = generator<xoshiro_8x64, plus<2, 0>>;
using xoshiro_8x64_plus_plus    = generator<xoshiro_8x64, plus_plus<17, 2, 0>>;
using xoshiro_8x64_star_star    = generator<xoshiro_8x64, star_star<5, 7, 9, 1>>;

// The analysed versions of the xoroshiro generators:
using xoroshiro_2x32_star       = generator<xoroshiro_2x32,  star<0x9E3779BB, 0>>;
using xoroshiro_2x32_star_star  = generator<xoroshiro_2x32,  star_star<0x9E3779BB, 5, 5, 0>>;
using xoroshiro_2x64_plus       = generator<xoroshiro_2x64,  plus<0, 1>>;
using xoroshiro_2x64_plus_plus  = generator<xoroshiro_2x64b, plus_plus<17, 0, 1>>;
using xoroshiro_2x64_star_star  = generator<xoroshiro_2x64,  star_star<5, 7, 9, 0>>;
using xoroshiro_16x64_plus_plus = generator<xoroshiro_16x64, plus_plus<23, 15, 0>>;
using xoroshiro_16x64_star      = generator<xoroshiro_16x64, star<0x9e3779b97f4a7c13, 0>>;
using xoroshiro_16x64_star_star = generator<xoroshiro_16x64, star_star<5, 7, 9, 0>>;
// clang-format on

/// The recommended 32-bit output generator -- used as `xso::rng32`.
using rng32 = xoshiro_4x32_star_star;

/// The recommended 64-bit output generator -- used as `xso::rng64`.
using rng64 = xoshiro_4x64_star_star;

/// The recommended default generator for most usage -- used as `xso::rng`.
using rng = rng64;

// --------------------------------------------------------------------------------------------------------------------
// Partitions of random number stream:
//
// For parallel processing applications it can be useful to split a single random number stream into a number of
// non-overlapping "partitions". Then different computational threads can get their "own set" of random numbers without
// worrying about stream overlaps etc.
//
// The partitions need to be very large so this only works for States where we can jump far ahead in an efficient way.
// --------------------------------------------------------------------------------------------------------------------

/// Partition a random number stream into a number of non-overlapping sub-streams.
///
/// The `RNG` template parameter should be a PRNG that defines an appropriate jump method.
///
/// Given a parent random number generator we can split its random number stream into a number of non-overlapping
/// partitions. The substreams are just independent random number generators of the same type as the parent but seeded
/// at the start of each partition. Each call to the partition's `next()` method returns the next such generator.
template<typename RNG>
class partition {
public:
    /// Constructs a _partition_ for the passed parent random number generator `rng`.
    ///
    /// The stream from the parent `rng` is split into `n_partitions` non-overlapping sub-streams where each sub-stream
    /// is as large as possible. The substreams are actually independent random number generators of the same type as
    /// the paren but seeded at the start of each partition. Each call to the `next()` method returns the next such
    /// generator.
    ///
    /// **NOTE:** The requested number of partitions is silently adjusted if it is zero.
    partition(const RNG& rng, std::size_t n_partitions) : m_rng{rng} {
        // Make sure the requested number of partitions makes sense -- silently fix any issues.
        if (n_partitions == 0) n_partitions = 1;

        // How many bits of state in the RNG type?
        auto n_bits = RNG::bit_count();

        // The period of the generator is `2^n_bits` so each partition ideally has size `2^n_bits / n_partitions`.
        // That number will probably overflow a std::size_t so we must instead keep everything in log 2 form.
        // First we find the smallest `n` such that `2^n >= n_partitions - 1`.
        // Note if `n_partitions` is 128 the following gives `n = 7` and does the same if `n = 100`.
        auto n = static_cast<std::size_t>(std::bit_width(n_partitions - 1));

        // We will create `2^n` partitions which is probably more than needed but the wastage is negligible.
        // To create those `2^n` partitions we must be able to jump ahead `2^(n_bits - n)` steps many times.
        auto power_2 = n_bits - n;

        // Precompute the jump coefficients to advance the generator `2^power_2` steps i.e. along to the next partition.
        m_jump_coefficients = xso::jump_coefficients<typename RNG::state_type>(power_2, true);
    }

    /// Returns the next generator that is seeded at the start of the next sub-stream of the parent random number
    /// stream.
    RNG next() {
        // We already have a pre-baked generator seeded at the right spot ready to go.
        RNG result = m_rng;

        // Prep for the next call by jumping the parent copy once more along its stream.
        m_rng.jump(m_jump_coefficients);

        // And return the pre-baked one ...
        return result;
    }

private:
    using array_type = typename RNG::array_type;

    RNG        m_rng;
    array_type m_jump_coefficients;
};

// --------------------------------------------------------------------------------------------------------------------
// Non-member functions for computing jump polynomials:
// --------------------------------------------------------------------------------------------------------------------

/// Returns the coefficients of the _jump polynomial_ that can be used to jump a generator/state ahead by
/// $J = n$ or $J= 2^n$ steps. The coefficients are compactly packed into the returned array of words.
///
/// The jump polynomial is computed $r(x) = x^J \mathrm{mod} c(x)$ where $c(x)$ is the characteristic polynomial of
/// the state transition matrix.
///
/// The computed _jump polynomial_ $r(x) = r_0 + r_1 x + ... + r_{n-1} x^{n-1}$ can be represented by its
/// $n$ bit coefficients. Those coefficients are returned compactly packed into an array of words.
///
/// # Panics
/// This method depends on having the State's precomputed characteristic polynomial available to us. In practice, we
/// have precomputed the characteristic polynomial for just a few xoshiro/xoroshiro's with specific parameters.
/// If the `State` has no precomputed characteristic coefficients this method will fail at compile time.
template<typename State>
constexpr auto
jump_coefficients(std::size_t n, bool n_is_log2) {

    // Retrieve the low order coefficients of characteristic polynomial --- this can throw if not precomputed.
    std::array<typename State::word_type, State::word_count()> char_coefficients;
    State::characteristic_coefficients(char_coefficients.begin());

    // The jump polynomial is x^J mod c(x) -- that computation expects to be handed p(x) where c(x) = x^n + p(x).
    return xso::reduce(char_coefficients, n, n_is_log2);
}

// --------------------------------------------------------------------------------------------------------------------
// We have some "internal" functions that reduce x^J mod c(x) --- needed for the jump polynomial computations.
// These re-implement a more general reduction method in the gf2::BitPolynomial class (https://nessan/github.io/gf2/).
// Repeated here to make `xoshiro.h` self-contained.
//
// Not part of the public API but used for the jump calculations in the generator class.
// --------------------------------------------------------------------------------------------------------------------

// Function to "riffle" a `src` word into two other words `lo` and `hi` containing the bits from `src`
// interleaved with zeros.
//
// So the 8-bit word `abcdefgh` fills `lo` as `a0b0c0d0` and `hi` as  `e0f0g0h0`.
//
// # Example
// ```
// std::uint8_t src = 0b1111'1111;
// std::uint8_t lo, hi;
// xso::riffle(src, lo, hi);
// assert_eq(lo, 0b01010101, "lo is {:08b}", lo);
// assert_eq(hi, 0b01010101, "hi is {:08b}", hi);
// ```
// *NOTE:** This function is "public" for testing purposes only -- it is not part of the normal interface.
template<std::unsigned_integral word_type>
constexpr void
riffle(word_type src, word_type& lo, word_type& hi) {
    // Constants
    constexpr std::size_t bits_per_word = std::numeric_limits<word_type>::digits;
    constexpr std::size_t half_bits = bits_per_word / 2;
    constexpr word_type   one{1};
    constexpr word_type   ones = std::numeric_limits<word_type>::max();

    // Split the src into lo and hi halves.
    lo = src & (ones >> half_bits);
    hi = src >> half_bits;

    // Some magic to interleave the respective halves with zeros.
    for (auto i = bits_per_word / 4; i > 0; i /= 2) {
        word_type div = word_type(one << i) | one;
        word_type msk = ones / div;
        lo = (lo ^ (lo << i)) & msk;
        hi = (hi ^ (hi << i)) & msk;
    }
}

// Function that riffles a `src` array of unsigneds into two others `lo` and `hi` which will be filled with the
// bits from `src` interleaved with zeros.
//
// We treat `lo` and `hi` as contiguous storage and fill the elements of `lo` first and then `hi`.
// This allows us to optionally reuse `src` for the output array `lo` -- the call `riffle(src, src, hi)` will work.
//
// # Example
// ```
// std::array<std::uint8_t, 2> src = {0b1111'1111, 0b1111'1111};
// std::array<std::uint8_t, 2> lo, hi;
// xso::riffle(src, lo, hi);
// assert_eq(lo, (std::array<std::uint8_t, 2>{0b01010101, 0b01010101}));
// assert_eq(hi, (std::array<std::uint8_t, 2>{0b01010101, 0b01010101}));
// ```
// *NOTE:** This function is "public" for testing purposes only -- it is not part of the normal interface.
template<std::unsigned_integral word_type, std::size_t N>
static constexpr void
riffle(const std::array<word_type, N>& src, std::array<word_type, N>& lo, std::array<word_type, N>& hi) {
    // We will riffle each word in src into two other words x & y
    word_type x, y;

    // We work through `src` in reverse order -- this allows the reuse of `src` for `lo`!
    // Treating [lo|hi] as contiguous storage, first working back through hi and then back through lo.
    for (std::size_t i = N; i-- > 0;) {
        riffle(src[i], x, y);
        if (2 * i + 1 > N) {
            // Both x & y go in hi -- note that if 2i + 1 - N > 0 then 2i - N is >= 0.
            hi[2 * i - N] = x;
            hi[2 * i + 1 - N] = y;
        } else if (2 * i + 1 == N) {
            // Straddling situation where y goes in the first word of hi and x in the last word of lo.
            lo[N - 1] = x;
            hi[0] = y;
        } else {
            // Need to pop both x & y into the lo array.
            lo[2 * i] = x;
            lo[2 * i + 1] = y;
        }
    }
}

// Function that computes r(x) = x^e mod c(x) in GF(2) where e = J or 2^J and J is an unsigned integer argument.
//
// The polynomial c(x) is monic of degree n so c(x) = x^n + p(x) where p(x) is a polynomial of degree less than n.
// We are passed the n coefficients of p(x) packed into an array of unsigned integers.
// We return the coefficients of r(x) = x^e mod c(x) packed into an array of unsigned integers in the same format.
//
// # Note
// Any linting tool you use will (reasonably) complain that the complexity of this method is too high!
// The method is a direct re-implementation of the `gf2::BitPolynomial::reduce` method which is also complex but
// easier to understand as it is properly factored. The version here is is less general as it assumes that the
// degree of p(x) is a multiple of 32.
//
// *NOTE:** This method "public" for testing purposes only -- it is not part of the normal interface.
template<std::unsigned_integral word_type, std::size_t N>
constexpr auto
reduce(const std::array<word_type, N>& p, std::size_t J, bool J_is_log2) {

    // We will return the coefficients of r(x) = x^e mod c(x) packed into an array of unsigned integers of this type.
    using array_type = std::array<word_type, N>;

    // Constant we use to indicate "no such position"/"not found" and a couple of others.
    constexpr auto      npos = static_cast<std::size_t>(-1);
    constexpr word_type one{1};

    // The polynomial c(x) = x^n + p(x) where p(x) = p_0 + p_1 x + ... + p_{n-1} x^{n-1} and n is:
    constexpr std::size_t bits_per_word = std::numeric_limits<word_type>::digits;
    constexpr std::size_t n = N * bits_per_word;

    // lambda: Returns the index of the word that holds p_i.
    auto word = [=](std::size_t i) { return i / bits_per_word; };

    // lambda: Returns the bit location of p_i inside the word that holds it.
    auto bit = [=](std::size_t i) { return i % bits_per_word; };

    // lambda: Returns a mask that isolates p_i within the word that holds it.
    auto mask = [=](std::size_t i) { return word_type{one << bit(i)}; };

    // lambda: Returns true if poly_i is 1.
    auto test = [=](const auto& poly, std::size_t i) -> bool { return poly[word(i)] & mask(i); };

    // lambda: Sets poly_i to 1.
    auto set = [=](auto& poly, std::size_t i) { poly[word(i)] |= mask(i); };

    // lambda: Returns the index for the least significant set bit in the argument or `npos` if none set.
    auto lsb = [=](word_type w) { return w == 0 ? npos : static_cast<std::size_t>(std::countr_zero(w)); };

    // lambda: Returns the index for the most significant set bit in the argument or `npos` if none set.
    auto msb = [=](word_type w) { return static_cast<std::size_t>(std::bit_width(w) - 1); };

    // lambda: Returns the first set coefficient in poly or `npos` if the coefficients are all zero.
    auto first_set = [=](const auto& poly) {
        for (auto i = 0uz; i < N; ++i)
            if (poly[i] != 0) return i * bits_per_word + lsb(poly[i]);
        return npos;
    };

    // lambda: Returns the final set coefficient in poly or `npos` if the coefficients are all zero.
    auto final_set = [=](const auto& poly) {
        std::size_t i = N;
        while (i--)
            if (poly[i] != 0) return i * bits_per_word + msb(poly[i]);
        return npos;
    };

    // lambda: Returns true if the highest coefficient in `poly` is set (then poly is said to be monic).
    auto monic = [=](const auto& poly) {
        constexpr std::size_t complement = bits_per_word - 1;
        constexpr auto        final_bit_mask = word_type{one << complement};
        return poly[N - 1] & final_bit_mask;
    };

    // lambda: Computes lhs <- lhs + rhs which in GF(2) is equivalent to  lhs <- lhs^rhs.
    auto add = [=](auto& lhs, const auto& rhs) {
        for (auto i = 0uz; i < N; ++i) lhs[i] ^= rhs[i];
    };

    // lambda: Performs a one place shift on the polynomial coefficients stored poly.
    // Shift is to the the right if you think the elements are in vector order [p0,p1,p2,p3] -> [0,p0,p1,p2].
    // Shift is to the left when you think in bit order [b3,b2,b1,b0] -> [b2,b1,b0,0].
    auto shift = [=](auto& poly) {
        constexpr std::size_t complement = bits_per_word - 1;
        for (std::size_t i = N - 1; i > 0; --i) {
            auto l = static_cast<word_type>(poly[i] << 1);
            auto r = static_cast<word_type>(poly[i - 1] >> complement);
            poly[i] = l | r;
        }
        poly[0] <<= 1;
    };

    // lambda: If degree[poly] < n, this performs poly(x) <- x*poly(x) mod c(x) where c(x) = x^n + p(x).
    auto times_x_step = [&](auto& poly) {
        bool add_p = monic(poly);
        shift(poly);
        if (add_p) add(poly, p);
    };

    // We precompute x^{n + i} mod c(x) for i = 0, ..., n-1 starting from the known x^n mod c(x) = p.
    // Each x^{n + i} mod c(x) is a word-vector of coefficients and we put the lot into a std::vector.
    std::vector<array_type> power_mod(n);
    power_mod[0] = p;
    for (std::size_t i = 1; i < n; ++i) {
        power_mod[i] = power_mod[i - 1];
        times_x_step(power_mod[i]);
    }

    // Some work space we use below.
    array_type hi;

    // lambda: If degree[poly] < n, performs: poly(x) <- poly(x)^2 mod c(x) where as usual c(x) = x^n + p(x).
    auto square_step = [&](auto& poly) {
        // Compute poly(x)^2 -- in GF(2) this means interspersing all the coefficients with zeros.
        // We actually riffle poly directly into two arrays lo & hi so that poly(x)^2 = lo(x) + x^n hi(x).
        // This only works because we assume n some multiple of N (i.e. all bits in poly matter).
        // NOTE: Our riffle method above for arrays lets us reuse the poly array for lo.
        riffle(poly, poly, hi);

        // poly(x)^2 mod c(x) now is poly(x) + x^n hi(x) mod c(x) as degree[poly] < n.
        // Add the x^n hi(x) mod c(x) term-by-term noting that at most every second term in hi(x) is 1.
        auto hi_first = first_set(hi);
        if (hi_first != npos) {
            auto hi_final = final_set(hi);
            for (std::size_t i = hi_first; i <= hi_final; i += 2)
                if (test(hi, i)) add(poly, power_mod[i]);
        }
    };

    // Initialize our return value to all zeros.
    array_type result;
    result.fill(0);

    // Case: e = 2^J -- we just do J squaring steps starting from r(x) = x to get to  x^(2^J) mod c(x).
    if (J_is_log2) {
        set(result, 1);
        for (std::size_t j = 0; j < J; ++j) square_step(result);
        return result;
    }

    // Case e = J < n: Then x^J mod c(x) = x^J so we can set the appropriate coefficient in r and return.
    if (J < n) {
        set(result, J);
        return result;
    }

    // Case e = J = n: Then x^J mod c(x) = p(x).
    if (J == n) return p;

    // Case e = J > n: We use a square & multiply algorithm:
    // Note that if e.g. J = 0b00010111 then std::bit_floor(J) = 0b00010000.
    std::size_t J_bit = std::bit_floor(J);

    // Start with r(x) = x mod c(x) which takes care of the most significant binary digit in J.
    set(result, 1);
    J_bit >>= 1;

    // And off we go ...
    while (J_bit) {

        // Always do a square step and then a times_x step if necessary (i.e. if current bit in J is set).
        square_step(result);
        if (J & J_bit) times_x_step(result);

        // On to the next bit position in n.
        J_bit >>= 1;
    }
    return result;
}

#ifdef GF2

// --------------------------------------------------------------------------------------------------------------------
// Non-member functions that are only defined if we have access to the `gf2` library.
// See https://nessan.github.io/gf2/ for tools for working with polynomials and matrices over GF(2).
// --------------------------------------------------------------------------------------------------------------------

/// Returns the transition matrix for a state/generator type as a `gf2::BitMatrix`.
///
/// # Note
/// The transition matrix will be a square `n x n` matrix over GF(2) where `n` is the number of bits in the state.
/// The state advance operation can be represented as a matrix-vector multiplication over GF(2)
/// $\mathbf{s}\leftarrow T \cdot \mathbf{s},$ where $\mathbf{s}$ is the state vector, and $T$ is the transition
/// matrix.
///
/// This method computes that $T$ matrix by examining the action of the `step()` method on all the unit states.
template<typename State>
auto
transition_matrix() {

    using word_type = typename State::word_type;

    // Some constants for our state size.
    constexpr auto n_words = State::word_count();
    constexpr auto n_bits = State::bit_count();

    // The transition matrix will be a square n_bits x n_bits matrix over GF(2).
    gf2::BitMatrix<word_type> result{n_bits, n_bits};

    // Some work-space in word and bit space
    gf2::BitVector<word_type>      bits{n_bits};
    std::array<word_type, n_words> words;

    // Create an State instance we will use to step through unit states.
    State state;

    // We get the columns of the transition matrix by looking  at the action of `step()` on all the unit states.
    for (std::size_t k = 0; k < n_bits; ++k) {

        // Create the k'th unit state (i.e. the state just has the k'th bit set and all others are zero)
        bits.set_all(false);
        bits.set(k, true);

        // Translate that unit bit-vector into an array of words.
        bits.to_words(words.begin());

        // Seed the state from those words.
        state.seed(words.cbegin(), words.cend());

        // Advance the state by one step.
        state.step();

        // Translate the new state to bit-space.
        for (auto i = 0uz; i < n_words; ++i) bits.set_word(i, state[i]);

        // Store those bits into column `k` of the transition matrix.
        // NOTE:  Columnar access for a gf2::BitMatrix must be done element by element.
        for (auto i = 0uz; i < n_bits; ++i) result(i, k) = bits[i];
    }

    return result;
}

/// Returns the characteristic polynomial for our state's transition matrix as a `gf2::BitPolynomial`.
///
/// # Note
/// If the transition matrix is `n x n` then the return will have degree `n + 1` and should be monic.
template<typename State>
auto
characteristic_polynomial() {
    auto T = xso::transition_matrix<State>();
    return T.characteristic_polynomial();
}

/// Returns a jump polynomial that moves the generator type `J` steps ahead in its random number stream.
///
/// This method computes the jump polynomial $r(x) = x^J \mathrm{mod} c(x),$ where $c(x)$ is the characteristic
/// polynomial of the state transition matrix and $J$ is either $N$ or $2^N$.
///
/// Setting `N_is_log2` to true allows for really huge jumps like `J = 2^100` which would overflow normal
/// integer types.
template<typename State>
auto
jump_polynomial(gf2::BitPolynomial<typename State::word_type> const& c, std::size_t N, bool N_is_log2 = false) {
    return c.reduce_x_to_the(N, N_is_log2);
}

#endif

} // namespace xso

// --------------------------------------------------------------------------------------------------------------------
// Formatting and output stream support for our generators:
// --------------------------------------------------------------------------------------------------------------------

// A concept that matches any type that has an accessible `type_string()` class `method.
template<typename T>
concept has_type_string_class_method = requires {
    { T::type_string() } -> std::convertible_to<std::string>;
};

/// Our classes are connected to `std::format` and friends by specializing the `std::formatter` struct.
///
/// We simply forward format requests to each type's `type_string` method.
template<has_type_string_class_method T>
struct std::formatter<T> {

    /// Parse the format specifier -- currently only handle the default empty specifier
    constexpr auto parse(const std::format_parse_context& ctx) {
        auto it = ctx.begin();
        assert(it == ctx.end() || *it == '}');
        return it;
    }

    /// Push out a formatted xso::generator using its `type_string()` method.
    template<class FormatContext>
    auto format(const T&, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "{}", T::type_string());
    }
};

/// The usual output stream operator for an `xso::generator`, `State`, or `Scrambler`.
///
/// We simply forward output requests to each type's `type_string` method.
template<has_type_string_class_method T>
std::ostream&
operator<<(std::ostream& s, const T&) {
    s << T::type_string();
    return s;
}
