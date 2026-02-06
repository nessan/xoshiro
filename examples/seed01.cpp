// Nearby seeds do not produce nearby states.
//
// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <xoshiro.h>

int
main() {
    xso::rng g0(1), g1(2); // <1>
    std::println("After seeding with nearby seeds:");
    for (auto i = 0uz; i < xso::rng::word_count(); ++i) std::println("g0[{}] = {}\tg1[{}] = {}", i, g0[i], i, g1[i]);
}
