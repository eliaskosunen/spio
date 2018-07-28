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

#ifndef SPIO_STREAM_H
#define SPIO_STREAM_H

#include "config.h"

#include "filter.h"
#include "formatter.h"
#include "sink.h"
#include "source.h"
#include "stream_base.h"
#include "third_party/optional.h"

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
    public:
        output_stream_base() = default;
    };
    template <typename Device, typename Char>
    class output_stream_base<
        Device,
        Char,
        typename std::enable_if<is_sink<Device>::value &&
                                !is_writable<Device>::value>::type> {
    public:
        using formatter_type = basic_formatter<Char>;

        output_stream_base() = default;

        formatter_type formatter() const
        {
            return formatter_type{};
        }
    };
    template <typename Device, typename Char>
    class output_stream_base<
        Device,
        Char,
        typename std::enable_if<is_writable<Device>::value>::type> {
    public:
        using sink_type = guarded_buffered_writable<Device>;
        using formatter_type = basic_formatter<Char>;

        output_stream_base() = default;
        output_stream_base(sink_type s) : m_sink(std::move(s)) {}

        sink_type& sink()
        {
            Expects(m_sink.has_value());
            return *m_sink;
        }
        const sink_type& sink() const
        {
            Expects(m_sink.has_value());
            return *m_sink;
        }

        nonstd::optional<sink_type>& sink_storage()
        {
            return m_sink;
        }

        formatter_type formatter() const
        {
            return formatter_type{};
        }

    private:
        nonstd::optional<sink_type> m_sink;
    };

    template <typename Device, typename Enable = void>
    class input_stream_base {
    public:
        input_stream_base() = default;
    };
    template <typename Device>
    class input_stream_base<
        Device,
        typename std::enable_if<is_readable<Device>::value>::type> {
    public:
        using source_type = basic_buffered_readable<Device>;

        input_stream_base() = default;
        input_stream_base(source_type s) : m_source(std::move(s)) {}

        source_type& source() &
        {
            Expects(m_source.has_value());
            return *m_source;
        }
        const source_type& source() const&
        {
            Expects(m_source.has_value());
            return *m_source;
        }

        nonstd::optional<source_type> source_storage()
        {
            return m_source;
        }

    private:
        nonstd::optional<source_type> m_source;
    };
}  // namespace detail

template <typename Device, typename Char, template <typename...> class Chain>
class stream : public stream_base,
               public detail::input_stream_base<Device>,
               public detail::output_stream_base<Device, typename Char::type> {
public:
    using input_base = detail::input_stream_base<Device>;
    using output_base = detail::output_stream_base<Device, typename Char::type>;

    using character_type = Char;
    using char_type = typename Char::type;
    using chain_type = typename Char::template apply_filters<Chain>;
    using device_type = Device;

    class output_sentry {
        output_sentry(stream& s)
        {
            SPIO_UNUSED(s);
        }
    };
    class input_sentry {
        input_sentry(stream& s)
        {
            SPIO_UNUSED(s);
        }
    };

    stream(device_type d, input_base i, output_base o, chain_type c)
        : input_base(std::move(i)),
          output_base(std::move(o)),
          m_chain(std::move(c)),
          m_device(std::move(d))
    {
    }

    virtual bool is_open() const
    {
        return device().is_open();
    }
    virtual nonstd::expected<void, failure> close()
    {
        return device().close();
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

protected:
    stream(device_type d, chain_type c)
        : m_chain(std::move(c)), m_device(std::move(d))
    {
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
    return s.device().write(buf);
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
    if (r.value() < static_cast<std::ptrdiff_t>(buf.size()) || r.has_error()) {
        return r;
    }
    return s.device().write_at(buf, pos);
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
    return s.device().put(data);
}

template <typename Stream>
result flush(Stream& s)
{
    return s.sink().flush();
}
template <typename Stream>
nonstd::expected<void, failure> sync(Stream& s)
{
    return s.device().sync();
}

template <typename Stream>
result read(Stream& s, gsl::span<gsl::byte> data)
{
    bool eof = false;
    auto r = s.source().read(data, eof);
    if (r.has_error()) {
        putback(s, data.first(r.value()));
        return make_result(0, r.error());
    }
    if (eof) {
        s.set_eof();
    }
    data = data.first(r.value());
    r = s.chain().read(data);
    if (r.has_error()) {
        putback(s, data);
        return make_result(0, r.error());
    }
    if (r.value() < data.size()) {
        putback(s, data.subspan(r.value()));
    }
    return r;
}

template <typename Stream>
result read_at(Stream& s, gsl::span<gsl::byte> data, streampos pos)
{
    bool eof = false;
    auto r = s.device().read_at(data, pos, eof);
    if (r.has_error()) {
        putback(s, data.first(r.value()));
        return make_result(0, r.error());
    }
    if (eof) {
        s.set_eof();
    }
    data = data.first(r.value());
    r = s.chain().read(data);
    if (r.has_error()) {
        putback(s, data);
        return make_result(0, r.error());
    }
    if (r.value() < data.size()) {
        putback(s, data.subspan(r.value()));
    }
    return r;
}

template <typename Stream>
result get(Stream& s, gsl::byte& data)
{
    bool eof = false;
    auto r = s.device().get(data, eof);
    if (r.has_error()) {
        if (r.value() == 1) {
            putback(s, data);
        }
        return make_result(0, r.error());
    }
    if (eof) {
        s.set_eof();
    }
    r = s.chain().get(data);
    if (r.has_error()) {
        if (r.value() == 1) {
            putback(s, data);
        }
        return make_result(0, r.error());
    }
    return r;
}

template <typename Stream>
auto putback(Stream& s, gsl::span<const gsl::byte> d) ->
    typename std::enable_if<is_readable<Stream>::value, bool>::type
{
    s.clear_eof();
    return s.source().putback(d);
}
template <typename Stream>
auto putback(Stream& s, gsl::byte d) ->
    typename std::enable_if<is_byte_readable<Stream>::value &&
                                !is_readable<Stream>::value,
                            bool>::type
{
    s.clear_eof();
    return s.device().putback(d);
}

template <typename Stream>
nonstd::expected<streampos, failure> seek(Stream& s,
                                          streampos pos,
                                          inout which = in | out)
{
    return s.device().seek(pos, which);
}
template <typename Stream>
nonstd::expected<streampos, failure> seek(Stream& s,
                                          streamoff off,
                                          seekdir dir,
                                          inout which = in | out)
{
    return s.device().seek(off, dir, which);
}
template <typename Stream>
nonstd::expected<streampos, failure> tell(Stream& s, inout which = in | out)
{
    return seek(s, 0, seekdir::cur, which);
}

template <typename Stream>
using is_writable_stream = is_writable<typename Stream::device_type>;
template <typename Stream>
using is_random_access_writable_stream =
    is_random_access_writable<typename Stream::device_type>;
template <typename Stream>
using is_byte_writable_stream = is_byte_writable<typename Stream::device_type>;

template <typename Stream>
using is_flushable_stream = is_writable<typename Stream::device_type>;
template <typename Stream>
using is_syncable_stream = is_syncable<typename Stream::device_type>;

template <typename Stream>
using is_readable_stream = is_readable<typename Stream::device_type>;
template <typename Stream>
using is_random_access_readable_stream =
    is_random_access_readable<typename Stream::device_type>;
template <typename Stream>
using is_byte_readable_stream = is_byte_readable<typename Stream::device_type>;

template <typename Stream>
using is_putbackable_span_stream = is_readable_stream<Stream>;
template <typename Stream>
using is_putbackable_byte_stream =
    conjunction<is_byte_readable_stream<Stream>,
                negation<is_putbackable_span_stream<Stream>>>;

template <typename Stream>
using is_absolute_seekable_stream = is_detected<absolute_seekable_op, Stream>;
template <typename Stream>
using is_relative_seekable_stream = is_detected<relative_seekable_op, Stream>;

template <typename Stream>
using is_tellable_stream = is_seekable<typename Stream::device_type>;

SPIO_END_NAMESPACE

#endif  // SPIO_STREAM_H
