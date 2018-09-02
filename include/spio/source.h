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

namespace spio {
SPIO_BEGIN_NAMESPACE

template <typename Readable>
result read_all(Readable& d, span<byte> s, bool& eof)
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
expected<span<typename VectorReadable::buffer_type>, failure> vread_all(
    span<typename VectorReadable::buffer_type> bufs,
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

        basic_buffered_source_base(source_type* s) : m_source(s) {}

        SPIO_CONSTEXPR14 source_type& get() noexcept
        {
            return *m_source;
        }
        SPIO_CONSTEXPR const source_type& get() const noexcept
        {
            return *m_source;
        }
        SPIO_CONSTEXPR14 source_type& operator*() noexcept
        {
            return get();
        }
        SPIO_CONSTEXPR const source_type& operator*() const noexcept
        {
            return get();
        }
        SPIO_CONSTEXPR14 source_type* operator->() noexcept
        {
            return m_source;
        }
        SPIO_CONSTEXPR const source_type* operator->() const noexcept
        {
            return m_source;
        }

    private:
        source_type* m_source;
    };

    template <typename T>
    SPIO_CONSTEXPR14 T round_up_multiple_of_two(T n, T multiple) noexcept
    {
        Expects(multiple > 0);
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

    basic_buffered_readable(readable_type& r,
                            size_type s = size_type(buffer_size),
                            size_type rs = -1)
        : base(std::addressof(r)),
          m_buffer(detail::round_up_power_of_two(s)),
          m_read_size(rs)
    {
        if (m_read_size == -1) {
            m_read_size = m_buffer.size() / 2;
        }
        m_read_size = detail::round_up_power_of_two(m_read_size);
        Expects(rs <= m_buffer.size());
    }

    SPIO_CONSTEXPR size_type free_space() const noexcept
    {
        return m_buffer.free_space();
    }
    SPIO_CONSTEXPR size_type in_use() const noexcept
    {
        return m_buffer.in_use();
    }
    SPIO_CONSTEXPR size_type size() const noexcept
    {
        return m_buffer.size();
    }

    result read(span<byte> s, bool& eof)
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
    result putback(span<byte> s)
    {
#if SPIO_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#endif
        if (s.size() > free_space()) {
            return make_result(m_buffer.write_tail(s.first(free_space())),
                               out_of_memory);
        }
        return m_buffer.write_tail(s);
#if SPIO_GCC
#pragma GCC diagnostic pop
#endif
    }

private:
    SPIO_CONSTEXPR14 size_type get_read_size(size_type n) const noexcept
    {
        return std::min(detail::round_up_multiple_of_two(n, m_read_size),
                        free_space());
    }

    result read_into_buffer(size_type n, bool& eof)
    {
        Expects(n <= free_space());

        size_type has_read = 0;
        auto buffer = m_buffer.direct_write(n);
        for (auto it = buffer.begin(); it != buffer.end(); ++it) {
            auto r = base::get().read(*it, eof);
            has_read += r.value();
            if (r.has_error()) {
                return {has_read, r.inspect_error()};
            }
            if (eof) {
                if (it != buffer.end()) {
                    ++it;  // can't use a ranged for because of this
                }
                break;
            }
        }
        m_buffer.move_head(-(n - has_read));
        return has_read;
    }
    size_type read_from_buffer(span<byte> s)
    {
        return m_buffer.read(s);
    }

    buffer_type m_buffer;
    size_type m_read_size;
    bool m_eof{false};
};

SPIO_END_NAMESPACE
}  // namespace spio

#endif  // SPIO_SINK_H
