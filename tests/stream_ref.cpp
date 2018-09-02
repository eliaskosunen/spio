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
    std::vector<spio::byte> buf(24);
    spio::memory_outstream stream(buf);
    spio::basic_stream_ref<spio::encoding<char>,
                           spio::random_access_writable_tag>
        ref(stream);
    const auto str = "Hello world!";
    auto ret = spio::write_at(
        ref, spio::as_bytes(spio::make_span(str, strlen(str))), 0);
    CHECK(!ret.has_error());
    CHECK_EQ(std::memcmp(str, stream.device().output().data(), strlen(str)), 0);
    ret = spio::print_at(ref, 12, str);
    CHECK(!ret.has_error());
    CHECK_EQ(
        std::memcmp(str, stream.device().output().data() + 12, strlen(str)), 0);
}
