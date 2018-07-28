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
