/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include "common.h"

int
main()
{
    xso::rng gen;

    std::array<int, 10> u;
    std::iota(u.begin(), u.end(), 0);
    std::print("Original: {}\n", u);

    gen.shuffle(u);
    std::print("Shuffled: {}\n", u);
}