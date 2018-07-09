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

#ifndef SPIO_STREAM_H
#define SPIO_STREAM_H

#include "config.h"

#include "filter.h"
#include "formatter.h"
#include "owned_device.h"
#include "sink.h"
#include "source.h"
#include "stream_base.h"

SPIO_BEGIN_NAMESPACE

template <typename CharT>
struct character {
    using type = CharT;

    template <template <typename...> class Chain>
    using apply_filters = Chain<>;
};

namespace detail {
    template <typename Device>
    class guarded_buffered_writable : public basic_buffered_writable<Device> {
        using base = basic_buffered_writable<Device>;

    public:
        template <typename... Args>
        guarded_buffered_writable(Args&&... a) : base(std::forward<Args>(a)...)
        {
        }

        guarded_buffered_writable(const guarded_buffered_writable&) = delete;
        guarded_buffered_writable& operator=(const guarded_buffered_writable&) =
            delete;
        guarded_buffered_writable(guarded_buffered_writable&&) noexcept =
            default;
        guarded_buffered_writable& operator=(
            guarded_buffered_writable&&) noexcept = default;

        ~guarded_buffered_writable()
        {
            if (base::use_buffering()) {
                base::flush();
            }
        }
    };

    template <typename Device, typename Char, typename Enable = void>
    class output_stream_base {
    };
    template <typename Device, typename Char>
    class output_stream_base<
        Device,
        Char,
        typename std::enable_if<is_sink<Device>::value>::type> {
    public:
        using sink_type =
            typename std::conditional<is_writable<Device>::value,
                                      guarded_buffered_writable<Device>,
                                      Device>::type;
        using formatter_type = basic_formatter<Char>;

        output_stream_base(sink_type s) : m_sink(std::move(s)) {}

        sink_type& sink() &
        {
            return m_sink;
        }
        const sink_type& sink() const&
        {
            return m_sink;
        }
        sink_type&& sink() &&
        {
            return std::move(m_sink);
        }

        formatter_type formatter() const
        {
            return formatter_type{};
        }

    private:
        sink_type m_sink;
    };

    template <typename Device, typename Enable = void>
    class input_stream_base {
    };
    template <typename Device>
    class input_stream_base<
        Device,
        typename std::enable_if<is_source<Device>::value>::type> {
    public:
        using source_type =
            typename std::conditional<is_readable<Device>::value,
                                      basic_buffered_readable<Device>,
                                      Device>::type;
        input_stream_base(source_type s) : m_source(std::move(s)) {}

        source_type& source() &
        {
            return m_source;
        }
        const source_type& source() const&
        {
            return m_source;
        }
        source_type&& source() &&
        {
            return std::move(m_source);
        }

    private:
        source_type m_source;
    };
}  // namespace detail

template <typename Device, typename Char, template <typename...> class Chain>
class stream : public stream_base,
               public detail::input_stream_base<Device>,
               public detail::output_stream_base<Device, typename Char::type> {
public:
    using input_base = detail::input_stream_base<Device>;
    using output_base = detail::output_stream_base<Device, typename Char::type>;

    using char_type = typename Char::type;
    using chain_type = typename Char::template apply_filters<Chain>;
    using device_type = Device;

    stream(device_type d, input_base i, output_base o, chain_type c)
        : input_base(std::move(i)),
          output_base(std::move(o)),
          m_chain(std::move(c)),
          m_device(std::move(d))
    {
    }

    chain_type& chain()
    {
        return m_chain;
    }
    const chain_type& chain() const
    {
        return m_chain;
    }

    device_type& device() &
    {
        return m_device;
    }
    const device_type& device() const&
    {
        return m_device;
    }
    device_type&& device() &&
    {
        return std::move(m_device);
    }

private:
    chain_type m_chain;
    device_type m_device;
};

template <typename Stream>
result write(Stream& s, std::vector<gsl::byte> buf)
{
    auto r = s.chain().write(buf);
    if (r.value() < static_cast<std::ptrdiff_t>(buf.size()) || r.has_error()) {
        return r;
    }
    if (s.sink().use_buffering()) {
        return s.sink().write(buf);
    }
    return s.sink()->write(buf);
}
template <typename Stream>
result write(Stream& s, gsl::span<const gsl::byte> data)
{
    std::vector<gsl::byte> buf(data.begin(), data.end());
    return write(s, buf);
}

template <typename Stream>
result write_at(Stream& s, std::vector<gsl::byte> buf, streampos pos)
{
    auto r = s.chain().write(buf);
    if (r.value() < buf.size() || r.has_error()) {
        return r;
    }
    return s.sink().write_at(buf, pos);
}
template <typename Stream>
result write_at(Stream& s, gsl::span<const gsl::byte> data, streampos pos)
{
    std::vector<gsl::byte> buf(data.begin(), data.end());
    return write_at(s, buf, pos);
}

template <typename Stream>
result put(Stream& s, gsl::byte data)
{
    auto r = s.chain().put(data);
    if (r.value() != 1 || r.has_error()) {
        return r;
    }
    return s.sink().put(data);
}

template <typename Stream>
result flush(Stream& s)
{
    return s.sink().flush();
}

template <typename Stream>
result read(Stream& s, gsl::span<gsl::byte> data, bool& eof)
{
    auto r = s.source().read(data, eof);
    if (r.has_error()) {
        s.source().putback(data.first(r.value()));
        return make_result(0, r.error());
    }
    data = data.first(r.value());
    r = s.chain().read(data);
    if (r.has_error()) {
        s.source().putback(data);
        return make_result(0, r.error());
    }
    if (r.value() < data.size()) {
        s.source().putback(data.subspan(r.value()));
    }
    return r;
}

template <typename Stream>
result read_at(Stream& s, gsl::span<gsl::byte> data, streampos pos, bool& eof)
{
    auto r = s.source().read_at(data, pos, eof);
    if (r.has_error()) {
        s.source().putback(data.first(r.value()));
        return make_result(0, r.error());
    }
    data = data.first(r.value());
    r = s.chain().read(data);
    if (r.has_error()) {
        s.source().putback(data);
        return make_result(0, r.error());
    }
    if (r.value() < data.size()) {
        s.source().putback(data.subspan(r.value()));
    }
    return r;
}

template <typename Stream>
result get(Stream& s, gsl::byte& data, bool& eof)
{
    auto r = s.source().get(data, eof);
    if (r.has_error()) {
        if (r.value() == 1) {
            s.source().putback(data);
        }
        return make_result(0, r.error());
    }
    r = s.chain().get(data);
    if (r.has_error()) {
        if (r.value() == 1) {
            s.source().putback(data);
        }
        return make_result(0, r.error());
    }
    return r;
}

SPIO_END_NAMESPACE

#endif  // SPIO_STREAM_H
