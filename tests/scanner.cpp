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

#include <spio/spio.h>
#include "doctest.h"

TEST_CASE("scanner")
{
    std::string data{"true"};
    spio::memory_instream in(spio::as_bytes(spio::make_span(
        data.data(), static_cast<std::ptrdiff_t>(data.size()))));
    spio::basic_stream_ref<spio::encoding<char>,
                           spio::random_access_readable_tag>
        ref(in);

    SUBCASE("char")
    {
        char t{};
        char r{};
        char u{};
        char e{};
        auto ret = spio::scan_at(ref, 0, "{}{}{}{}", t, r, u, e);
        CHECK(ret.operator bool());
        CHECK(t == 't');
        CHECK(r == 'r');
        CHECK(u == 'u');
        CHECK(e == 'e');
    }
    SUBCASE("char span")
    {
        std::array<char, 4> arr{{0}};
        auto s = spio::span<char>(arr);
        auto ret = spio::scan_at(ref, 0, "{}", s);
        CHECK(ret.operator bool());
        CHECK_EQ(std::equal(data.begin(), data.end(), arr.begin()), true);
    }
    SUBCASE("bool")
    {
        bool val{};
        auto ret = spio::scan_at(ref, 0, "{}", val);
        CHECK(ret.operator bool());
        if (!ret) {
            puts(ret.error().what());
        }
        CHECK(val);
    }
}

TEST_CASE("scanner int")
{
    std::string data{"420"};
    spio::memory_instream in(spio::as_bytes(spio::make_span(
        data.data(), static_cast<std::ptrdiff_t>(data.size()))));
    spio::basic_stream_ref<spio::encoding<char>,
                           spio::random_access_readable_tag>
        ref(in);

    int val{};
    auto ret = spio::scan_at(ref, 0, "{}", val);
    CHECK(ret.operator bool());
    if (!ret) {
        puts(ret.error().what());
    }
    CHECK(val == 420);
}

TEST_CASE("scanner double")
{
    std::string data{"3.14159"};
    spio::memory_instream in(spio::as_bytes(spio::make_span(
        data.data(), static_cast<std::ptrdiff_t>(data.size()))));
    spio::basic_stream_ref<spio::encoding<char>,
                           spio::random_access_readable_tag>
        ref(in);

    double val{};
    auto ret = spio::scan_at(ref, 0, "{}", val);
    CHECK(ret.operator bool());
    if (!ret) {
        puts(ret.error().what());
    }
    CHECK(val == doctest::Approx(3.14159));
}
