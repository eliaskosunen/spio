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

namespace spio {
SPIO_BEGIN_NAMESPACE
template class basic_ring<char>;
SPIO_END_NAMESPACE
}  // namespace spio

TEST_CASE("ring")
{
    SUBCASE("construct")
    {
        spio::ring r(10);
        CHECK(r.empty());
        CHECK(r.size() >= 10);
        CHECK(r.get_span().size() == r.size());
        CHECK(r.in_use() == 0);
        CHECK(r.free_space() == r.size());
    }

    SUBCASE("basic operations")
    {
        spio::ring r(1024);
        char str[] = "Hello world!";
        auto strspan =
            spio::make_span(str, static_cast<std::ptrdiff_t>(sizeof str) - 1);

        CHECK(r.write(spio::as_bytes(strspan)) == strspan.size());
        CHECK(r.in_use() == strspan.size());
        CHECK(!r.empty());

        CHECK(r.read(spio::as_writeable_bytes(strspan)) == strspan.size());
        CHECK_EQ(std::strcmp(strspan.data(), "Hello world!"), 0);
        CHECK(r.in_use() == 0);
        CHECK(r.empty());

        CHECK(r.write(spio::as_bytes(strspan)) == strspan.size());
        CHECK(r.in_use() == strspan.size());
        CHECK(!r.empty());

        r.clear();
        CHECK(r.in_use() == 0);
        CHECK(r.empty());
    }

    SUBCASE("multiple writes")
    {
        spio::ring r(1024);
        char str[] = "Hello world!";
        auto strspan =
            spio::make_span(str, static_cast<std::ptrdiff_t>(sizeof str) - 1);

        CHECK(r.write(spio::as_bytes(strspan)) == strspan.size());
        CHECK(r.in_use() == strspan.size());
        CHECK(!r.empty());

        strspan[1] = 'a';
        CHECK(r.write(spio::as_bytes(strspan)) == strspan.size());
        CHECK(r.in_use() == strspan.size() * 2);

        std::string readbuf(static_cast<std::size_t>(strspan.size() * 2), '\0');
        CHECK(r.read(spio::as_writeable_bytes(spio::make_span(
                  &readbuf[0], static_cast<std::ptrdiff_t>(readbuf.size())))) ==
              readbuf.size());
        CHECK_EQ(std::strcmp(readbuf.data(), "Hello world!Hallo world!"), 0);
        CHECK(r.in_use() == 0);
        CHECK(r.empty());
    }

    SUBCASE("write tail")
    {
        spio::ring r(1024);
        char str[] = "Hello world!";
        auto strspan =
            spio::make_span(str, static_cast<std::ptrdiff_t>(sizeof str) - 1);

        CHECK(r.write(spio::as_bytes(strspan)) == strspan.size());
        CHECK(r.in_use() == strspan.size());
        CHECK(!r.empty());

        std::array<char, 4> tailwrite{{'1', '2', '3', '4'}};
        auto tailspan = spio::make_span(tailwrite);
        CHECK(r.write_tail(spio::as_bytes(tailspan)) == tailspan.size());
        CHECK(r.in_use() == strspan.size() + tailspan.size());

        std::string readbuf(static_cast<std::size_t>(r.in_use()), '\0');
        CHECK(r.read(spio::as_writeable_bytes(spio::make_span(
                  &readbuf[0], static_cast<std::ptrdiff_t>(readbuf.size())))) ==
              readbuf.size());
        CHECK_EQ(std::strcmp(readbuf.data(), "1234Hello world!"), 0);
        CHECK(r.empty());
    }

    SUBCASE("direct")
    {
        spio::ring r(1024);
        char str[] = "Hello world";
        auto strspan = spio::as_writeable_bytes(
            spio::make_span(str, static_cast<std::ptrdiff_t>(sizeof str) - 1));

        {
            auto it = strspan.begin();
            for (auto s : r.direct_write(strspan.size())) {
                std::copy(it, it + s.size(), s.begin());
                it += s.size();
            }
            CHECK(it == strspan.end());
            CHECK(r.in_use() == strspan.size());
        }
        {
            auto it = strspan.begin();
            for (auto s : r.direct_read(strspan.size())) {
                it = std::copy(s.begin(), s.end(), it);
            }
            CHECK(it == strspan.end());
            CHECK(r.empty());
        }
    }
}
