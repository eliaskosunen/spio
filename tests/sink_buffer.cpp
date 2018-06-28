// Copyright 2017-2018 Elias Kosunen
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <spio/spio.h>
#include "doctest.h"

TEST_CASE("sink_buffer")
{
    std::vector<gsl::byte> container;
    spio::vector_sink sink(container);

    SUBCASE("construction and getters: full")
    {
        spio::basic_buffered_writable<spio::vector_sink> buf(
            std::move(sink), spio::buffer_mode::full);
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
            std::move(sink), spio::buffer_mode::line);
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
            std::move(sink), spio::buffer_mode::none);
        CHECK(buf.size() == 0);
        CHECK(buf.free_space() == buf.size());
        CHECK(buf.empty());
        CHECK(buf.size() - buf.free_space() == buf.in_use());
        CHECK(!buf.use_buffering());
    }
    SUBCASE("construction and getters: external")
    {
        spio::basic_buffered_writable<spio::vector_sink> buf(
            std::move(sink), spio::buffer_mode::external);
        CHECK(buf.size() == 0);
        CHECK(buf.free_space() == buf.size());
        CHECK(buf.empty());
        CHECK(buf.size() - buf.free_space() == buf.in_use());
        CHECK(!buf.use_buffering());
    }

    SUBCASE("full buffering")
    {
        spio::basic_buffered_writable<spio::vector_sink> buf(
            std::move(sink), spio::buffer_mode::full);
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
            std::move(sink), spio::buffer_mode::line);
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
