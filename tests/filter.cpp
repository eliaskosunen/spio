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

struct nullify_output_filter : spio::output_filter {
    spio::result write(buffer_type& data) override
    {
        std::fill(data.begin(), data.end(), spio::to_byte(0));
        return static_cast<size_type>(data.size());
    }
};

struct nullify_input_filter : spio::input_filter {
    spio::result read(buffer_type& data) override
    {
        std::fill(data.begin(), data.end(), spio::to_byte(0));
        return data.size();
    }
};

TEST_CASE("sink_filter")
{
    spio::sink_filter_chain chain;
    CHECK(chain.size() == 0);
    CHECK(chain.empty());

    chain.push<spio::null_output_filter>();
    CHECK(chain.size() == 1);
    CHECK(!chain.empty());

    auto str = "Hello world!";
    auto len = std::strlen(str);
    std::vector<spio::byte> buffer(
        reinterpret_cast<const spio::byte*>(str),
        reinterpret_cast<const spio::byte*>(str) + len);

    CHECK_EQ(std::strcmp(str, reinterpret_cast<char*>(buffer.data())), 0);
    auto r = chain.write(buffer);
    CHECK(r.value() == len);
    CHECK(!r.has_error());
    CHECK_EQ(std::strcmp(str, reinterpret_cast<char*>(buffer.data())), 0);

    chain.push<nullify_output_filter>();
    CHECK(chain.size() == 2);

    r = chain.write(buffer);
    CHECK(r.value() == len);
    CHECK(!r.has_error());
    CHECK(std::strlen(reinterpret_cast<char*>(buffer.data())) == 0);
    for (auto& b : buffer) {
        CHECK(b == spio::to_byte(0));
    }
}

TEST_CASE("source_filter")
{
    spio::source_filter_chain chain;
    CHECK(chain.size() == 0);
    CHECK(chain.empty());

    chain.push<spio::null_input_filter>();
    CHECK(chain.size() == 1);
    CHECK(!chain.empty());

    auto str = "Hello world!";
    auto len = std::strlen(str);
    std::vector<spio::byte> buffer(
        reinterpret_cast<const spio::byte*>(str),
        reinterpret_cast<const spio::byte*>(str) + len);
    spio::vector_source source(buffer);

    std::vector<spio::byte> dest(len);
    auto d = spio::make_span(dest);

    bool eof = false;
    source.read(spio::as_writeable_bytes(spio::make_span(
                    dest.data(), static_cast<std::ptrdiff_t>(dest.size()))),
                eof);
    CHECK(eof);

    auto r = chain.read(d);
    CHECK(r.value() == len);
    CHECK(dest.size() == len);
    CHECK(!r.has_error());

    CHECK(buffer.size() == dest.size());
    CHECK_EQ(std::memcmp(buffer.data(), dest.data(), dest.size()), 0);

    chain.push<nullify_input_filter>();
    CHECK(chain.size() == 2);

    r = chain.read(d);
    CHECK(r.value() == len);
    CHECK(!r.has_error());

    CHECK(std::strlen(reinterpret_cast<char*>(dest.data())) == 0);
    size_t i = 0;
    for (auto& b : dest) {
        CHECK(b == spio::to_byte(0));
        CHECK(buffer[i] != b);
        ++i;
    }
}
