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

TEST_CASE("sink_buffer")
{
    std::vector<gsl::byte> container;
    spio::vector_sink sink(container);

    SUBCASE("construction and getters: full")
    {
        spio::basic_buffered_writable<spio::vector_sink> buf(
            sink, spio::buffer_mode::full);
        CHECK(buf.size() == BUFSIZ);
        CHECK(buf.free_space() == buf.size());
        CHECK(buf.empty());
        CHECK(buf.empty() != buf.full());
        CHECK(buf.size() - buf.free_space() == buf.in_use());
        CHECK(buf.use_buffering());
    }
    SUBCASE("construction and getters: line")
    {
        spio::basic_buffered_writable<spio::vector_sink> buf(
            sink, spio::buffer_mode::line);
        CHECK(buf.size() == BUFSIZ);
        CHECK(buf.free_space() == buf.size());
        CHECK(buf.empty());
        CHECK(buf.empty() != buf.full());
        CHECK(buf.size() - buf.free_space() == buf.in_use());
        CHECK(buf.use_buffering());
    }
    SUBCASE("construction and getters: none")
    {
        spio::basic_buffered_writable<spio::vector_sink> buf(
            sink, spio::buffer_mode::none);
        CHECK(buf.size() == 0);
        CHECK(buf.free_space() == buf.size());
        CHECK(buf.empty());
        CHECK(buf.size() - buf.free_space() == buf.in_use());
        CHECK(!buf.use_buffering());
    }
    SUBCASE("construction and getters: external")
    {
        spio::basic_buffered_writable<spio::vector_sink> buf(
            sink, spio::buffer_mode::external);
        CHECK(buf.size() == 0);
        CHECK(buf.free_space() == buf.size());
        CHECK(buf.empty());
        CHECK(buf.size() - buf.free_space() == buf.in_use());
        CHECK(!buf.use_buffering());
    }

    SUBCASE("full buffering")
    {
        spio::basic_buffered_writable<spio::vector_sink> buf(
            sink, spio::buffer_mode::full);
        std::vector<char> write(static_cast<size_t>(buf.size()));
        fill_random(write.begin(), write.end());

        bool flush = false;
        auto ret = buf.write(
            gsl::as_bytes(gsl::make_span(
                write.data(), static_cast<std::ptrdiff_t>(write.size()))),
            flush);
        CHECK(!flush);
        CHECK(ret.value() == write.size());
        CHECK(container.empty());
        CHECK(!ret.has_error());
        CHECK(buf.full());

        ret = buf.write(gsl::as_bytes(gsl::make_span(write.data(), 1)), flush);
        CHECK(flush);
        CHECK(ret.value() == 1);
        CHECK(container.size() == write.size());
        CHECK_EQ(std::memcmp(container.data(), write.data(), container.size()),
                 0);
        CHECK(!ret.has_error());
        CHECK(buf.in_use() == 1);

        ret = buf.flush();
        CHECK(ret.value() == 1);
        CHECK(!ret.has_error());
        CHECK(container.size() == write.size() + 1);
        CHECK(gsl::to_integer<char>(container.back()) == write.front());
    }

    SUBCASE("line buffering")
    {
        spio::basic_buffered_writable<spio::vector_sink> buf(
            sink, spio::buffer_mode::line);
        std::string write = "Hello world!\n";

        bool flush = false;
        auto ret = buf.write(
            gsl::as_bytes(gsl::make_span(
                write.data(), static_cast<std::ptrdiff_t>(write.size()) - 1)),
            flush);
        CHECK(!flush);
        CHECK(ret.value() == write.size() - 1);
        CHECK(container.empty());
        CHECK(!ret.has_error());
        CHECK(buf.in_use() == ret.value());

        ret = buf.write(gsl::as_bytes(gsl::make_span(&write.back(), 1)), flush);
        CHECK(flush);
        CHECK(ret.value() == 1);
        CHECK(container.size() == write.size());
        CHECK_EQ(std::memcmp(container.data(), write.data(), container.size()),
                 0);
        CHECK(!ret.has_error());
        CHECK(buf.empty());
    }
}
