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

TEST_CASE("source_buffer")
{
    std::vector<gsl::byte> container = [&]() {
        std::string str =
            "Hello world!\nHello another line!\nfoobar qwerty 1234567890!";
        auto s = gsl::as_writeable_bytes(gsl::make_span(
            &*str.begin(), static_cast<std::ptrdiff_t>(str.size())));
        return std::vector<gsl::byte>(s.begin(), s.end());
    }();
    spio::vector_source source(container);
    spio::basic_buffered_readable<spio::vector_source> buf(
        source, static_cast<std::ptrdiff_t>(BUFSIZ) * 2);
    const std::ptrdiff_t size = buf.size();

    SUBCASE("preconditions and getters")
    {
        CHECK(buf.in_use() == 0);
        CHECK(buf.free_space() == size);

        CHECK(std::addressof(buf.get()) == std::addressof(*buf));
        CHECK(buf->container() == buf.get().container());
    }

    SUBCASE("single full read")
    {
        std::vector<gsl::byte> read(container.size());
        bool eof = false;
        auto ret = buf.read(gsl::make_span(read), eof);
        CHECK(read.size() == container.size());
        CHECK(ret.value() == container.size());
        CHECK(!ret.has_error());
        CHECK(read == container);
        CHECK(buf.free_space() == size);
        CHECK(buf.in_use() == 0);
    }

    SUBCASE("basic putback")
    {
        std::vector<gsl::byte> read(container.size());
        bool eof = false;
        auto ret = buf.read(gsl::make_span(read).first(1), eof);
        CHECK(ret.value() == 1);
        CHECK(!ret.has_error());
        CHECK(read[0] == container[0]);
        CHECK(buf.in_use() == container.size() - 1);
        CHECK(buf.free_space() ==
              size - static_cast<std::ptrdiff_t>(container.size()) + 1);

        ret = buf.putback(gsl::make_span(read).first(1));
        CHECK(ret.value() == 1);
        CHECK(!ret.has_error());
        CHECK(buf.in_use() == container.size());
        CHECK(buf.free_space() ==
              size - static_cast<std::ptrdiff_t>(container.size()));

        ret = buf.read(gsl::make_span(read), eof);
        CHECK(!ret.has_error());
        CHECK(read == container);
        CHECK(buf.free_space() == size);
        CHECK(buf.in_use() == 0);
    }
}
