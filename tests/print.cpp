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

        spio::print_at(stream, 0, "Hello world!\n");

        CHECK_EQ(std::memcmp(buf.data(), "Hello world!\n", 12), 0);
    }
}
