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

#include "device.h"
#include "error.h"
#include "result.h"
#include "ring.h"
#include "third_party/expected.h"
#include "third_party/gsl.h"
#include "util.h"

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

template <typename T>
T round_up_multiple_of_two(T n, T multiple)
{
    Expects(multiple);
    Expects((multiple & (multiple - 1)) == 0);
    return (n + multiple - 1) & -multiple;
}
}  // namespace detail

template <typename Readable>
class basic_buffered_readable
    : public detail::basic_buffered_source_base<Readable> {
    using base = detail::basic_buffered_source_base<Readable>;

public:
    using readable_type = typename base::source_type;
    using buffer_type = ring;
    using size_type = std::ptrdiff_t;

    static SPIO_CONSTEXPR_DECL const size_type buffer_size = BUFSIZ * 2;

    basic_buffered_readable(readable_type&& r,
                            size_type s = buffer_size,
                            size_type rs = -1)
        : base(std::move(r)),
          m_buffer(detail::round_up_power_of_two(s)),
          m_read_size(rs)
    {
        if (m_read_size == -1) {
            m_read_size = m_buffer.size() / 2;
        }
        m_read_size = detail::round_up_power_of_two(m_read_size);
        Expects(rs <= m_buffer.size());
    }

    size_type free_space() const
    {
        return m_buffer.free_space();
    }
    size_type in_use() const
    {
        return m_buffer.in_use();
    }
    size_type size() const
    {
        return m_buffer.size();
    }

    result read(gsl::span<gsl::byte> s, bool& eof)
    {
        auto r = [&]() -> result {
            if (in_use() < s.size() && !m_eof) {
                return read_into_buffer(get_read_size(s.size()), m_eof);
            }
            return {0};
        }();
        if (m_eof && s.size() > in_use()) {
            eof = true;
        }
        s = s.first(std::min(s.size(), in_use()));
        auto bytes_read = read_from_buffer(s);
        Ensures(bytes_read == s.size());
        return {bytes_read, r.inspect_error()};
    }
    result putback(gsl::span<gsl::byte> s)
    {
        if (s.size() > free_space()) {
            return {m_buffer.write_tail(s), out_of_memory};
        }
        return m_buffer.write_tail(s);
    }

private:
    size_type get_read_size(size_type n)
    {
        return std::min(detail::round_up_multiple_of_two(n, m_read_size),
                        free_space());
    }

    result read_into_buffer(size_type n, bool& eof)
    {
        Expects(n <= free_space());
        auto has_read = 0;
        for (auto s : m_buffer.direct_write(n)) {
            auto r = base::get().read(s, eof);
            has_read += r.value();
            if (r.has_error()) {
                return {has_read, r.inspect_error()};
            }
        }
        m_buffer.move_head(-(n - has_read));
        return has_read;
    }
    size_type read_from_buffer(gsl::span<gsl::byte> s)
    {
        return m_buffer.read(s);
    }

    buffer_type m_buffer;
    size_type m_read_size;
    bool m_eof{false};
};

SPIO_END_NAMESPACE

#endif  // SPIO_SINK_H
