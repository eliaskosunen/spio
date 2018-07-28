// Copyright 2017-2018 Elias Kosunen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is a part of spio:
//     https://github.com/eliaskosunen/spio

#ifndef SPIO_DOCTEST_H
#define SPIO_DOCTEST_H

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <random>

template <typename It>
void fill_random(It begin, It end)
{
    using value_type = typename std::iterator_traits<It>::value_type;
    static std::random_device dev;
    static std::mt19937 eng(dev());

    std::uniform_int_distribution<long> dist(
        std::numeric_limits<value_type>::min(),
        std::numeric_limits<value_type>::max());
    std::generate(begin, end, [&]() { return static_cast<value_type>(dist(eng)); });
}

#endif  // SPIO_DOCTEST_H
