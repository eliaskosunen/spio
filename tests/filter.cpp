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

struct nullify_sink_filter : spio::sink_filter {
    spio::result write(buffer_type& data, size_type n, sink_type& sink) override
    {
        std::fill_n(data.begin(), n, gsl::to_byte(0));
        return sink(data, n);
    }
};

TEST_CASE("sink_filter")
{
    spio::sink_filter_chain chain;
    chain.push<spio::null_sink_filter>();

    auto str = "Hello world!";
    auto len = std::strlen(str);
    std::vector<gsl::byte> buffer(
        reinterpret_cast<const gsl::byte*>(str),
        reinterpret_cast<const gsl::byte*>(str) + len);

    auto fn = spio::sink_filter_chain::sink_type(
        [](spio::sink_filter_chain::buffer_type& buf,
           spio::sink_filter_chain::size_type n) {
            SPIO_UNUSED(buf);
            return n;
        });

    CHECK_EQ(std::strcmp(str, reinterpret_cast<char*>(buffer.data())), 0);
    auto r = chain.write(buffer, static_cast<std::ptrdiff_t>(len), fn);
    CHECK(r.value() == len);
    CHECK(!r.has_error());
    CHECK_EQ(std::strcmp(str, reinterpret_cast<char*>(buffer.data())), 0);

    chain.push<nullify_sink_filter>();
    r = chain.write(buffer, static_cast<std::ptrdiff_t>(len), fn);
    CHECK(r.value() == len);
    CHECK(!r.has_error());
    CHECK(std::strlen(reinterpret_cast<char*>(buffer.data())) == 0);
    for (auto& b : buffer) {
        CHECK(b == gsl::to_byte(0));
    }
}

TEST_CASE("source_filter")
{
    spio::source_filter_chain chain;
    chain.push<spio::null_source_filter>();

    auto str = "Hello world!";
    auto len = std::strlen(str);
    std::vector<gsl::byte> buffer(
        reinterpret_cast<const gsl::byte*>(str),
        reinterpret_cast<const gsl::byte*>(str) + len);
    spio::vector_source source(buffer);

    std::vector<gsl::byte> dest(len);
    auto fn = spio::source_filter_chain::source_type(
        [&source](spio::source_filter_chain::buffer_type& buf,
                  spio::sink_filter_chain::size_type n, bool& eof) {
            return source.read(gsl::make_span(buf.data(), n), eof);
        });
    chain.read(dest, dest.size(), fn);

    CHECK(buffer.size() == dest.size());
    CHECK_EQ(std::memcmp(buffer.data(), dest.data(), dest.size()), 0);
}

#if 0
struct nullify_sink_filter : spio::sink_filter {
    using size_type = sink_filter::size_type;

    virtual spio::result write(gsl::span<const gsl::byte> input,
                               gsl::span<gsl::byte> output) override
    {
        std::fill(output.begin(), output.end(), gsl::to_byte(0));
        return input.size();
    }
};

TEST_CASE("sink_filter")
{
    spio::sink_filter_chain chain;
    chain.filters().push_back(spio::make_unique<spio::null_sink_filter>());

    auto str = "Hello world!";
    auto len = std::strlen(str);
    std::vector<gsl::byte> buffer(
        reinterpret_cast<const gsl::byte*>(str),
        reinterpret_cast<const gsl::byte*>(str) + len);

    for (auto f : chain.iterable<std::vector<gsl::byte>>()) {
        f = buffer;
        CHECK(f.get_result().value() == len);
        CHECK(!f.get_result().has_error());
    }
    CHECK_EQ(std::strcmp(str, reinterpret_cast<char*>(buffer.data())), 0);

    chain.filters().push_back(spio::make_unique<nullify_sink_filter>());
    for (auto f : chain.iterable<std::vector<gsl::byte>>()) {
        f = buffer;
        CHECK(f.get_result().value() == len);
        CHECK(!f.get_result().has_error());
    }
    CHECK(std::strlen(reinterpret_cast<char*>(buffer.data())) == 0);
}
#endif
