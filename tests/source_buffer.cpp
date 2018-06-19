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

TEST_CASE("source_buffer")
{
    std::vector<gsl::byte> container = [&]() {
        std::string str =
            "Hello world!\nHello another line!\nfoobar qwerty 1234567890!";
        auto s =
            gsl::as_writeable_bytes(gsl::make_span(&*str.begin(), str.size()));
        return std::vector<gsl::byte>(s.begin(), s.end());
    }();
    spio::vector_source source(container);
    std::vector<gsl::byte> buffer(container.size());
    spio::basic_buffered_readable<spio::vector_source> buf(std::move(source),
                                                           buffer);

    SUBCASE("preconditions and getters")
    {
        CHECK(buf.free_begin() == 0);
        CHECK(buf.in_use() == 0);
        CHECK(buf.free_end() == buffer.size());

        CHECK(std::addressof(buf.get()) == std::addressof(*buf));
        CHECK(buf->container() == buf.get().container());
    }

    SUBCASE("single full read")
    {
        std::vector<gsl::byte> read(container.size());
        bool eof = false;
        buf.read(gsl::make_span(read), eof);
        CHECK(eof);
        CHECK(read.size() == container.size());
        CHECK(read == container);
    }
}
