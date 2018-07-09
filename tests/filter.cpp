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

struct nullify_output_filter : spio::output_filter {
    spio::result write(buffer_type& data) override
    {
        std::fill(data.begin(), data.end(), gsl::to_byte(0));
        return static_cast<size_type>(data.size());
    }
};

struct nullify_input_filter : spio::input_filter {
    spio::result read(buffer_type& data) override
    {
        std::fill(data.begin(), data.end(), gsl::to_byte(0));
        return data.size();
    }
};

TEST_CASE("sink_filter")
{
    spio::sink_filter_chain<> chain;
    CHECK(chain.get_dynamic().size() == 0);
    CHECK(chain.get_dynamic().empty());

    chain.get_dynamic().push<spio::null_output_filter>();
    CHECK(chain.get_dynamic().size() == 1);
    CHECK(!chain.get_dynamic().empty());

    auto str = "Hello world!";
    auto len = std::strlen(str);
    std::vector<gsl::byte> buffer(
        reinterpret_cast<const gsl::byte*>(str),
        reinterpret_cast<const gsl::byte*>(str) + len);

    CHECK_EQ(std::strcmp(str, reinterpret_cast<char*>(buffer.data())), 0);
    auto r = chain.write(buffer);
    CHECK(r.value() == len);
    CHECK(!r.has_error());
    CHECK_EQ(std::strcmp(str, reinterpret_cast<char*>(buffer.data())), 0);

    chain.get_dynamic().push<nullify_output_filter>();
    CHECK(chain.get_dynamic().size() == 2);

    r = chain.write(buffer);
    CHECK(r.value() == len);
    CHECK(!r.has_error());
    CHECK(std::strlen(reinterpret_cast<char*>(buffer.data())) == 0);
    for (auto& b : buffer) {
        CHECK(b == gsl::to_byte(0));
    }
}

TEST_CASE("source_filter")
{
    spio::source_filter_chain<> chain;
    CHECK(chain.get_dynamic().size() == 0);
    CHECK(chain.get_dynamic().empty());

    chain.get_dynamic().push<spio::null_input_filter>();
    CHECK(chain.get_dynamic().size() == 1);
    CHECK(!chain.get_dynamic().empty());

    auto str = "Hello world!";
    auto len = std::strlen(str);
    std::vector<gsl::byte> buffer(
        reinterpret_cast<const gsl::byte*>(str),
        reinterpret_cast<const gsl::byte*>(str) + len);
    spio::vector_source source(buffer);

    std::vector<gsl::byte> dest(len);
    auto d = gsl::make_span(dest);

    bool eof = false;
    source.read(gsl::as_writeable_bytes(gsl::make_span(
                    dest.data(), static_cast<std::ptrdiff_t>(dest.size()))),
                eof);
    CHECK(eof);

    auto r = chain.read(d);
    CHECK(r.value() == len);
    CHECK(dest.size() == len);
    CHECK(!r.has_error());

    CHECK(buffer.size() == dest.size());
    CHECK_EQ(std::memcmp(buffer.data(), dest.data(), dest.size()), 0);

    chain.get_dynamic().push<nullify_input_filter>();
    CHECK(chain.get_dynamic().size() == 2);

    r = chain.read(d);
    CHECK(r.value() == len);
    CHECK(!r.has_error());

    CHECK(std::strlen(reinterpret_cast<char*>(dest.data())) == 0);
    size_t i = 0;
    for (auto& b : dest) {
        CHECK(b == gsl::to_byte(0));
        CHECK(buffer[i] != b);
        ++i;
    }
}
