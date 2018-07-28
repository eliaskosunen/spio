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

TEST_CASE("print")
{
    SUBCASE("stdout")
    {
        spio::stdio_sink sink(stdout);
        using stream_type =
            spio::stream<spio::stdio_sink, spio::character<char>,
                         spio::sink_filter_chain>;
        stream_type stream(
            sink, stream_type::input_base{},
            stream_type::output_base::sink_type(sink, spio::buffer_mode::none),
            stream_type::chain_type{});
        spio::print(stream, "Hello world\n");
        spio::print(stream, "Number: {}\n", 42);
    }

    SUBCASE("stdio_handle")
    {
        spio::basic_stdio_handle_iostream<spio::character<char>> s(
            stdout, spio::buffer_mode::full);
        spio::print(s, "Hello world\n");
        spio::print(s, "Number: {}\n", 42);
    }

    SUBCASE("memory_sink")
    {
        std::vector<gsl::byte> buf(12);
        spio::memory_sink sink(buf);
        using stream_type =
            spio::stream<spio::memory_sink, spio::character<char>,
                         spio::sink_filter_chain>;
        stream_type stream(sink, stream_type::input_base{},
                           stream_type::output_base{},
                           stream_type::chain_type{});

        auto ret = spio::print_at(stream, 0, "Hello world!\n");
        CHECK(!ret.has_error());

        CHECK_EQ(std::memcmp(buf.data(), "Hello world!\n", 12), 0);
    }
}
