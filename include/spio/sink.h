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

namespace spio {
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

        basic_buffered_sink_base(sink_type* s) : m_sink(s) {}

        SPIO_CONSTEXPR14 sink_type& get() noexcept
        {
            return *m_sink;
        }
        SPIO_CONSTEXPR const sink_type& get() const noexcept
        {
            return *m_sink;
        }
        SPIO_CONSTEXPR14 sink_type& operator*() noexcept
        {
            return get();
        }
        SPIO_CONSTEXPR const sink_type& operator*() const noexcept
        {
            return get();
        }
        SPIO_CONSTEXPR14 sink_type* operator->() noexcept
        {
            return m_sink;
        }
        SPIO_CONSTEXPR const sink_type* operator->() const noexcept
        {
            return m_sink;
        }

    private:
        sink_type* m_sink;
    };
}  // namespace detail

template <typename Writable>
class basic_buffered_writable
    : public detail::basic_buffered_sink_base<Writable> {
    using base = detail::basic_buffered_sink_base<Writable>;

public:
    using writable_type = typename base::sink_type;
    using buffer_type = std::vector<gsl::byte>;
    using size_type = std::ptrdiff_t;

    basic_buffered_writable(writable_type& w,
                            buffer_mode m,
                            size_type s = BUFSIZ)
        : base(std::addressof(w)), m_buf(_init_buffer(m, s)), m_mode(m)
    {
    }

    result write(gsl::span<const gsl::byte> s)
    {
        bool f = false;
        return write(s, f);
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
        Expects(use_buffering());

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

    SPIO_CONSTEXPR bool use_buffering() const noexcept
    {
        return _use_buffering(m_mode);
    }

    SPIO_CONSTEXPR size_type size() const noexcept
    {
        return static_cast<size_type>(m_buf.size());
    }
    SPIO_CONSTEXPR size_type in_use() const noexcept
    {
        return m_next;
    }
    SPIO_CONSTEXPR size_type free_space() const noexcept
    {
        return size() - in_use();
    }
    SPIO_CONSTEXPR bool empty() const noexcept
    {
        return in_use() == 0;
    }
    SPIO_CONSTEXPR bool full() const noexcept
    {
        return free_space() == 0;
    }

    SPIO_CONSTEXPR const buffer_type& buffer() const noexcept
    {
        return m_buf;
    }
    SPIO_CONSTEXPR buffer_mode mode() const noexcept
    {
        return m_mode;
    }

private:
    size_type write_to_buffer(gsl::span<const gsl::byte> s) noexcept
    {
        Expects(free_space() >= s.size());
        std::copy(s.begin(), s.end(), m_buf.begin() + m_next);
        m_next += s.size();
        return s.size();
    }

    static SPIO_CONSTEXPR bool _use_buffering(buffer_mode m) noexcept
    {
        return (static_cast<typename std::underlying_type<buffer_mode>::type>(
                    m) >>
                2) == 0;
    }
    static buffer_type _init_buffer(buffer_mode m, size_type s) noexcept
    {
        if (_use_buffering(m)) {
            return buffer_type(static_cast<std::size_t>(s));
        }
        return {};
    }

    buffer_type m_buf;
    size_type m_next{0};
    buffer_mode m_mode;
};

SPIO_END_NAMESPACE
}  // namespace spio

#endif  // SPIO_SINK_H
