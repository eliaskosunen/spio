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

#ifndef SPIO_SOURCE_H
#define SPIO_SOURCE_H

#include "config.h"

#include <vector>
#include "device.h"
#include "error.h"
#include "result.h"
#include "third_party/expected.h"
#include "third_party/gsl.h"

#ifndef SPIO_READ_ALL_MAX_ATTEMPTS
#define SPIO_READ_ALL_MAX_ATTEMPTS 8
#endif

SPIO_BEGIN_NAMESPACE

template <typename Readable>
result read_all(Readable& d, gsl::span<gsl::byte> s, bool& eof)
{
    auto total_read = 0;
    for (auto i = 0; i < SPIO_READ_ALL_MAX_ATTEMPTS; ++i) {
        auto ret = d.read(s, eof);
        total_read += ret.value();
        if (eof) {
            break;
        }
        if (SPIO_UNLIKELY(ret.has_error() &&
                          ret.error().code() != std::errc::interrupted)) {
            return make_result(total_read, ret.error());
        }
        if (SPIO_UNLIKELY(ret.value() != s.size())) {
            s = s.subspan(ret.value());
        }
        break;
    }
    return total_read;
}

template <typename VectorReadable>
nonstd::expected<gsl::span<typename VectorReadable::buffer_type>, failure>
vread_all(gsl::span<typename VectorReadable::buffer_type> bufs,
          streampos pos = 0)
{
    SPIO_UNUSED(bufs);
    SPIO_UNUSED(pos);
    SPIO_UNIMPLEMENTED;
}

template <typename Readable>
class buffered_readable {
public:
    using readable_type = Readable;
    using buffer_type = std::vector<gsl::byte>;
    using iterator = buffer_type::iterator;
    using size_type = buffer_type::size_type;

    buffered_readable(readable_type&& r, size_type n)
        : m_readable(std::move(r)),
          m_buffer(n),
          m_begin(m_buffer.begin()),
          m_next(m_buffer.begin()),
          m_end(m_buffer.begin() + 1)
    {
    }

    size_type in_use() const
    {
        return std::distance(m_begin, m_next);
    }
    void resize(size_type n)
    {
        Expects(n >= in_use());

        m_buffer.resize(n);
    }

    size_type space_left() const
    {
        return std::distance(m_next, m_end);
    }

    readable_type& get() SPIO_NOEXCEPT
    {
        return m_readable;
    }
    const readable_type& get() const SPIO_NOEXCEPT
    {
        return m_readable;
    }
    readable_type& operator*() SPIO_NOEXCEPT
    {
        return get();
    }
    const readable_type& operator*() const SPIO_NOEXCEPT
    {
        return get();
    }
    readable_type* operator->() SPIO_NOEXCEPT
    {
        return std::addressof(get());
    }
    const readable_type* operator->() const SPIO_NOEXCEPT
    {
        return std::addressof(get());
    }

private:
    result read_into_buffer(gsl::span<gsl::byte> s) {}

    readable_type m_readable;
    buffer_type m_buffer;
    iterator m_begin;
    iterator m_next;
    iterator m_end;
};

SPIO_END_NAMESPACE

#endif  // SPIO_SINK_H
