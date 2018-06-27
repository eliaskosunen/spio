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

#define SPIO_RING_USE_MMAP 1
#include <spio/spio.h>
#include "doctest.h"

SPIO_BEGIN_NAMESPACE
template class basic_ring<char>;
SPIO_END_NAMESPACE

TEST_CASE("ring")
{
    SUBCASE("construct")
    {
        spio::ring r(10);
        CHECK(r.empty());
        CHECK(r.size() >= 10);
        CHECK(r.span().size() == r.size());
        CHECK(r.in_use() == 0);
        CHECK(r.free_space() == r.size());
    }

    SUBCASE("basic operations")
    {
        spio::ring r(1024);
        char str[] = "Hello world!";
        auto strspan = gsl::make_span(str, std::strlen(str));

        CHECK(r.write(gsl::as_bytes(strspan)) == strspan.size());
        CHECK(r.in_use() == strspan.size());
        CHECK(!r.empty());

        CHECK(r.read(gsl::as_writeable_bytes(strspan)) == strspan.size());
        CHECK_EQ(std::strcmp(strspan.data(), "Hello world!"), 0);
        CHECK(r.in_use() == 0);
        CHECK(r.empty());

        CHECK(r.write(gsl::as_bytes(strspan)) == strspan.size());
        CHECK(r.in_use() == strspan.size());
        CHECK(!r.empty());

        r.clear();
        CHECK(r.in_use() == 0);
        CHECK(r.empty());
    }

    SUBCASE("multiple writes")
    {
        spio::ring r(1024);
        char str[] = "Hello world!";
        auto strspan = gsl::make_span(str, std::strlen(str));

        CHECK(r.write(gsl::as_bytes(strspan)) == strspan.size());
        CHECK(r.in_use() == strspan.size());
        CHECK(!r.empty());

        strspan[1] = 'a';
        CHECK(r.write(gsl::as_bytes(strspan)) == strspan.size());
        CHECK(r.in_use() == strspan.size() * 2);

        std::string readbuf(strspan.size() * 2, '\0');
        CHECK(r.read(gsl::as_writeable_bytes(gsl::make_span(
                  &readbuf[0], readbuf.size()))) == readbuf.size());
        CHECK_EQ(std::strcmp(readbuf.data(), "Hello world!Hallo world!"), 0);
        CHECK(r.in_use() == 0);
        CHECK(r.empty());
    }

    SUBCASE("write tail")
    {
        spio::ring r(1024);
        char str[] = "Hello world!";
        auto strspan = gsl::make_span(str, std::strlen(str));

        CHECK(r.write(gsl::as_bytes(strspan)) == strspan.size());
        CHECK(r.in_use() == strspan.size());
        CHECK(!r.empty());

        std::array<char, 4> tailwrite{{'1', '2', '3', '4'}};
        auto tailspan = gsl::make_span(tailwrite);
        CHECK(r.write_tail(gsl::as_bytes(tailspan)) == tailspan.size());
        CHECK(r.in_use() == strspan.size() + tailspan.size());

        std::string readbuf(r.in_use(), '\0');
        CHECK(r.read(gsl::as_writeable_bytes(gsl::make_span(&readbuf[0], readbuf.size()))) == readbuf.size());
        CHECK_EQ(std::strcmp(readbuf.data(), "1234Hello world!"), 0);
        CHECK(r.empty());
    }

    SUBCASE("direct")
    {
        spio::ring r(1024);
        char str[] = "Hello world";
        auto strspan =
            gsl::as_writeable_bytes(gsl::make_span(str, std::strlen(str)));

        {
            auto it = strspan.begin();
            for (auto s : r.direct_write(strspan.size())) {
                std::copy(it, it + s.size(), s.begin());
                it += s.size();
            }
            CHECK(it == strspan.end());
            CHECK(r.in_use() == strspan.size());
        }
        {
            auto it = strspan.begin();
            for (auto s : r.direct_read(strspan.size())) {
                it = std::copy(s.begin(), s.end(), it);
            }
            CHECK(it == strspan.end());
            CHECK(r.empty());
        }
    }
}
