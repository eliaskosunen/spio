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

TEST_CASE("stream_ref")
{
    std::vector<gsl::byte> buf(12);
    spio::memory_sink sink(buf);
    using stream_type = spio::stream<spio::memory_sink, spio::character<char>,
                                     spio::sink_filter_chain>;
    stream_type stream(sink, stream_type::input_base{},
                       stream_type::output_base{}, stream_type::chain_type{});
    spio::basic_stream_ref<spio::character<char>,
                           spio::random_access_writable_tag>
        ref(stream);
    const auto str = "Hello world!";
    spio::write_at(ref, gsl::as_bytes(gsl::make_span(str, strlen(str))), 0);
    CHECK_EQ(std::memcmp(str, stream.device().output().data(), strlen(str)), 0);
}
