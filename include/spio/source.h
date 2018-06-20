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

namespace detail {
template <typename Source>
class basic_buffered_source_base {
public:
    using source_type = Source;

    basic_buffered_source_base(source_type&& s) : m_source(std::move(s)) {}

    source_type& get() SPIO_NOEXCEPT
    {
        return m_source;
    }
    const source_type& get() const SPIO_NOEXCEPT
    {
        return m_source;
    }
    source_type& operator*() SPIO_NOEXCEPT
    {
        return get();
    }
    const source_type& operator*() const SPIO_NOEXCEPT
    {
        return get();
    }
    source_type* operator->() SPIO_NOEXCEPT
    {
        return std::addressof(get());
    }
    const source_type* operator->() const SPIO_NOEXCEPT
    {
        return std::addressof(get());
    }

private:
    source_type m_source;
};
}  // namespace detail

template <typename Readable>
class basic_buffered_readable
    : public detail::basic_buffered_source_base<Readable> {
    using base = detail::basic_buffered_source_base<Readable>;

public:
    using readable_type = typename base::source_type;
    using buffer_type = gsl::span<gsl::byte>;
    using iterator = buffer_type::iterator;
    using size_type = std::ptrdiff_t;

    basic_buffered_readable(readable_type&& r, buffer_type b)
        : basic_buffered_readable(std::move(r), b, b.size())
    {
    }
    basic_buffered_readable(readable_type&& r,
                            buffer_type b,
                            size_type lookahead)
        : base(std::move(r)),
          m_buffer(b),
          m_begin(m_buffer.begin()),
          m_next(m_buffer.begin()),
          m_lookahead(lookahead)
    {
        Expects(lookahead <= b.size());
    }

    size_type free_begin() const
    {
        return std::distance(m_buffer.begin(), m_begin);
    }
    size_type in_use() const
    {
        return std::distance(m_begin, m_next);
    }
    size_type free_end() const
    {
        return std::distance(m_next, m_buffer.end());
    }

    result read(gsl::span<gsl::byte> s, bool& eof)
    {
        auto n = read_from_buffer(
            s.first(std::min(in_use(), static_cast<size_type>(s.size()))));
        if (n == s.size()) {
            return n;
        }
        s = s.subspan(n);
        if (s.size() > free_end()) {
            push_backward();
        }
        auto r = read_into_buffer(s.size(), eof);
        read_from_buffer(s.first(r.value()));
        return {r.value() + n, r.inspect_error()};
    }
    result putback(gsl::span<gsl::byte> s)
    {
        if (free_begin() >= s.size()) {
            std::copy(s.begin(), s.end(), m_begin - s.size());
            m_begin -= s.size();
            return s.size();
        }
        push_forward();
        auto n = std::min(free_begin(), static_cast<size_type>(s.size()));
        std::copy(s.begin(), s.begin() + n, m_begin - n);
        m_begin -= n;
        if (n != s.size()) {
            return {n, out_of_memory};
        }
        return n;
    }

private:
    result read_into_buffer(size_type n, bool& eof)
    {
        Expects(n <= free_end());

        auto s = gsl::make_span(&*m_next, n);
        auto r = base::get().read(s, eof);
        m_next += r.value();
        return r;

        /* auto to_read = std::min(free_end(), std::max(n, m_lookahead)); */
        /* auto s = gsl::make_span(&*m_next, to_read); */
        /* auto r = base::get().read(s, eof); */
        /* m_next += r.value(); */
        /* return {std::min(r.value(), n), r.inspect_error()}; */
    }
    size_type read_from_buffer(gsl::span<gsl::byte> s)
    {
        Expects(s.size() <= in_use());
        std::copy(m_begin, m_begin + s.size(), s.begin());
        m_begin += s.size();
        return s.size();
    }
    void push_backward()
    {
        m_next = std::copy(m_begin, m_next, m_buffer.begin());
        m_begin = m_buffer.begin();

        Ensures(free_begin() == 0);
    }
    void push_forward()
    {
        m_next = std::copy(m_begin, m_next, m_begin + free_end());
        m_begin += free_end();

        Ensures(free_end() == 0);
    }

    buffer_type m_buffer;
    iterator m_begin;
    iterator m_next;
    size_type m_lookahead;
};

SPIO_END_NAMESPACE

#endif  // SPIO_SINK_H
