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
