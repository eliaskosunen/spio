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

#ifndef SPIO_SINK_H
#define SPIO_SINK_H

#include "config.h"

#include "device.h"
#include "error.h"
#include "result.h"
#include "third_party/expected.h"
#include "util.h"

#ifndef SPIO_WRITE_ALL_MAX_ATTEMPTS
#define SPIO_WRITE_ALL_MAX_ATTEMPTS 8
#endif

SPIO_BEGIN_NAMESPACE

template <typename Device>
result write_all(Device& d, gsl::span<const gsl::byte> s)
{
    auto total_written = 0;
    for (auto i = 0; i < SPIO_WRITE_ALL_MAX_ATTEMPTS; ++i) {
        auto ret = d.write(s);
        total_written += ret.value();
        if (SPIO_UNLIKELY(ret.has_error() &&
                          ret.error().code() != std::errc::interrupted)) {
            return make_result(total_written, ret.error());
        }
        if (SPIO_UNLIKELY(ret.value() != s.size())) {
            s = s.subspan(ret.value());
        }
        break;
    }
    return total_written;
}

template <typename Device>
nonstd::expected<gsl::span<typename Device::const_buffer_type>, failure>
vwrite_all(gsl::span<typename Device::const_buffer_type> bufs,
           streampos pos = 0)
{
    SPIO_UNUSED(bufs);
    SPIO_UNUSED(pos);
    SPIO_UNIMPLEMENTED;
}

enum class buffer_mode : std::uint8_t {
    line = 1,
    full = 2,
    none = 4,
    external = 8
};

namespace detail {
template <typename Sink>
class basic_buffered_sink_base {
public:
    using sink_type = Sink;

    basic_buffered_sink_base(sink_type&& s) : m_source(std::move(s)) {}

    sink_type& get() SPIO_NOEXCEPT
    {
        return m_source;
    }
    const sink_type& get() const SPIO_NOEXCEPT
    {
        return m_source;
    }
    sink_type& operator*() SPIO_NOEXCEPT
    {
        return get();
    }
    const sink_type& operator*() const SPIO_NOEXCEPT
    {
        return get();
    }
    sink_type* operator->() SPIO_NOEXCEPT
    {
        return std::addressof(get());
    }
    const sink_type* operator->() const SPIO_NOEXCEPT
    {
        return std::addressof(get());
    }

private:
    sink_type m_source;
};
}  // namespace detail

template <typename Writable>
class basic_buffered_writable
    : private detail::basic_buffered_sink_base<Writable> {
    using base = detail::basic_buffered_sink_base<Writable>;

public:
    using writable_type = typename base::sink_type;
    using buffer_type = std::vector<gsl::byte>;
    using size_type = std::ptrdiff_t;

    basic_buffered_writable(writable_type&& w,
                            buffer_mode m,
                            size_type s = BUFSIZ)
        : base(std::move(w)), m_mode(m), m_buf(_init_buffer(m, s))
    {
    }

    result write(gsl::span<const gsl::byte> s, bool& flushed)
    {
        Expects(use_buffering());

        if (m_mode == buffer_mode::full) {
            auto n = std::min(s.size(), free_space());
            write_to_buffer(s.first(n));
            s = s.subspan(n);
            if (s.empty()) {
                return n;
            }
            auto res = flush();
            flushed = true;
            if (res.has_error()) {
                return {n, res.inspect_error()};
            }
            return write(s, flushed);
        }

        // TODO: rewrite
        auto i = s.size() - 1;
        for (; i != -1; --i) {
            if (*(s.begin() + i) == gsl::to_byte('\n')) {
                break;
            }
        }
        auto first = i != -1 ? gsl::make_span(s.begin(), i + 1) : s;
        auto n = std::min(first.size(), free_space());
        std::copy(first.begin(), first.begin() + n, m_buf.begin() + m_next);
        m_next += n;
        if (full() || i != -1) {
            auto res = flush();
            flushed = true;
            if (res.has_error()) {
                return {n, res.inspect_error()};
            }
            return n;
        }
        s = s.subspan(n);
        if (s.empty()) {
            return n;
        }
        return write(s, flushed);
    }

    result flush()
    {
        auto res = base::get().write(gsl::make_span(m_buf.data(), in_use()));
        if (SPIO_LIKELY(res.value() == in_use())) {
            m_next = 0;
            return res;
        }
        if (res.value() == 0) {
            return res;
        }
        std::copy(m_buf.begin() + in_use() - res.value(),
                  m_buf.begin() + in_use(), m_buf.begin());
        m_next = res.value();
        return res;
    }

    bool use_buffering() const
    {
        return _use_buffering(m_mode);
    }

    size_type size() const
    {
        return m_buf.size();
    }
    size_type in_use() const
    {
        return m_next;
    }
    size_type free_space() const
    {
        return size() - in_use();
    }
    bool empty() const
    {
        return in_use() == 0;
    }
    bool full() const
    {
        return free_space() == 0;
    }

    const buffer_type& buffer() const
    {
        return m_buf;
    }
    buffer_mode mode() const
    {
        return m_mode;
    }

private:
    size_type write_to_buffer(gsl::span<const gsl::byte> s)
    {
        Expects(free_space() >= s.size());
        std::copy(s.begin(), s.end(), m_buf.begin() + m_next);
        m_next += s.size();
        return s.size();
    }

    static bool _use_buffering(buffer_mode m)
    {
        return (static_cast<typename std::underlying_type<buffer_mode>::type>(
                    m) >>
                2) == 0;
    }
    static buffer_type _init_buffer(buffer_mode m, size_type s)
    {
        if (_use_buffering(m)) {
            return buffer_type(s);
        }
        return {};
    }

    buffer_type m_buf;
    size_type m_next{0};
    buffer_mode m_mode;
};

SPIO_END_NAMESPACE

#endif  // SPIO_SINK_H
