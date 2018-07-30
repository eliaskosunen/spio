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

namespace spio {
SPIO_BEGIN_NAMESPACE

template <typename CharT>
struct character {
    using type = CharT;

    template <template <typename...> class Chain>
    using apply_filters = Chain<>;

    static SPIO_CONSTEXPR streampos to_device(streampos pos) noexcept
    {
        return pos.operator streamoff() * static_cast<streamoff>(sizeof(type));
    }
    static SPIO_CONSTEXPR streamoff to_device(streamoff off) noexcept
    {
        return off * static_cast<streamoff>(sizeof(type));
    }

    static SPIO_CONSTEXPR streampos from_device(streampos pos) noexcept
    {
        return pos.operator streamoff() / static_cast<streamoff>(sizeof(type));
    }
    static SPIO_CONSTEXPR streamoff from_device(streamoff off) noexcept
    {
        return off / static_cast<streamoff>(sizeof(type));
    }
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

        SPIO_CONSTEXPR output_stream_base() = default;

        SPIO_CONSTEXPR formatter_type formatter() const noexcept
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

        SPIO_CONSTEXPR output_stream_base() = default;
        SPIO_CONSTEXPR14 output_stream_base(sink_type s) noexcept
            : m_sink(std::move(s))
        {
        }

        SPIO_CONSTEXPR14 sink_type& sink() noexcept
        {
            Expects(m_sink.has_value());
            return *m_sink;
        }
        SPIO_CONSTEXPR14 const sink_type& sink() const noexcept
        {
            Expects(m_sink.has_value());
            return *m_sink;
        }

        SPIO_CONSTEXPR14 nonstd::optional<sink_type>& sink_storage() noexcept
        {
            return m_sink;
        }

        SPIO_CONSTEXPR formatter_type formatter() const noexcept
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

        SPIO_CONSTEXPR input_stream_base() = default;
        SPIO_CONSTEXPR input_stream_base(source_type s) : m_source(std::move(s))
        {
        }

        SPIO_CONSTEXPR14 source_type& source() & noexcept
        {
            Expects(m_source.has_value());
            return *m_source;
        }
        SPIO_CONSTEXPR14 const source_type& source() const& noexcept
        {
            Expects(m_source.has_value());
            return *m_source;
        }

        SPIO_CONSTEXPR14 nonstd::optional<source_type> source_storage()
        {
            return m_source;
        }

    private:
        nonstd::optional<source_type> m_source;
    };
}  // namespace detail

template <typename Char, typename Properties>
class basic_stream_ref;

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
    using tied_type = basic_stream_ref<Char, flushable_tag>;

    class output_sentry {
    public:
        output_sentry(stream& s)
        {
            if (!s) {
                m_result = nonstd::make_unexpected(sentry_error);
                s.set_bad();
                return;
            }
            auto t = _handle_tied(s);
            if (t.has_error()) {
                m_result = nonstd::make_unexpected(t.error());
                return;
            }
            if (!s) {
                m_result = nonstd::make_unexpected(sentry_error);
            }
        }

        SPIO_CONSTEXPR const failure& error() const noexcept
        {
            return m_result.error();
        }
        SPIO_CONSTEXPR explicit operator bool() const noexcept
        {
            return m_result.operator bool();
        }

    private:
        result _handle_tied(stream& s);

        nonstd::expected<void, failure> m_result{};
    };
    class input_sentry {
    public:
        input_sentry(stream& s, bool skipws = true)
        {
            if (!s) {
                m_result = nonstd::make_unexpected(sentry_error);
                s.set_bad();
                return;
            }
            auto t = _handle_tied(s);
            if (t.has_error()) {
                m_result = nonstd::make_unexpected(t.error());
                return;
            }
            if (skipws) {
                // TODO: skipws
                /* auto w = _skipws(s); */
                /* if(w.has_error()) { */
                /*     m_result = nonstd::make_unexpected(t.error()); */
                /*     return; */
                /* } */
            }
            if (!s) {
                m_result = nonstd::make_unexpected(sentry_error);
            }
        }

        SPIO_CONSTEXPR const failure& error() const noexcept
        {
            return m_result.error();
        }
        SPIO_CONSTEXPR explicit operator bool() const noexcept
        {
            return m_result.operator bool();
        }

    private:
        result _handle_tied(stream& s);

        nonstd::expected<void, failure> m_result{};
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

    SPIO_CONSTEXPR14 chain_type& chain() noexcept
    {
        return m_chain;
    }
    SPIO_CONSTEXPR const chain_type& chain() const noexcept
    {
        return m_chain;
    }

    SPIO_CONSTEXPR14 device_type& device() & noexcept
    {
        return m_device;
    }
    SPIO_CONSTEXPR const device_type& device() const& noexcept
    {
        return m_device;
    }
    SPIO_CONSTEXPR14 device_type&& device() && noexcept
    {
        return std::move(m_device);
    }

    SPIO_CONSTEXPR tied_type* tie() const noexcept
    {
        return m_tie;
    }
    SPIO_CONSTEXPR14 tied_type* tie(tied_type* s) noexcept
    {
        auto prev = m_tie;
        m_tie = s;
        return prev;
    }

protected:
    stream(device_type d, chain_type c)
        : m_chain(std::move(c)), m_device(std::move(d))
    {
    }

private:
    chain_type m_chain;
    device_type m_device;
    tied_type* m_tie{nullptr};
};

template <typename Stream>
auto write(Stream& s, std::vector<gsl::byte> buf) ->
    typename std::enable_if<is_writable_stream<Stream>::value, result>::type
{
    auto sentry = typename Stream::output_sentry(s);
    if (!sentry) {
        return make_result(0, sentry.error());
    }
    if (!s.chain().output_empty()) {
        auto r = s.chain().write(buf);
        if (r.value() < static_cast<std::ptrdiff_t>(buf.size()) ||
            r.has_error()) {
            return r;
        }
    }
    if (s.sink().use_buffering()) {
        return s.sink().write(buf);
    }
    return s.device().write(buf);
}
template <typename Stream>
result write(Stream& s, gsl::span<const gsl::byte> data)
{
    if (!s.chain().output_empty()) {
        std::vector<gsl::byte> buf(data.begin(), data.end());
        return write(s, buf);
    }
    if (s.sink().use_buffering()) {
        return s.sink().write(data);
    }
    return s.device().write(data);
}

template <typename Stream>
result write_at(Stream& s, std::vector<gsl::byte> buf, streampos pos)
{
    auto sentry = typename Stream::output_sentry(s);
    if (!sentry) {
        return make_result(0, sentry.error());
    }
    if (!s.chain().output_empty()) {
        auto r = s.chain().write(buf);
        if (r.value() < static_cast<std::ptrdiff_t>(buf.size()) ||
            r.has_error()) {
            return r;
        }
    }
    return s.device().write_at(buf, Stream::character_type::to_device(pos));
}
template <typename Stream>
result write_at(Stream& s, gsl::span<const gsl::byte> data, streampos pos)
{
    std::vector<gsl::byte> buf(data.begin(), data.end());
    return write_at(s, buf, pos);
}

template <typename Stream>
auto put(Stream& s, gsl::byte data) ->
    typename std::enable_if<is_byte_writable_stream<Stream>::value,
                            result>::type
{
    auto sentry = typename Stream::output_sentry(s);
    if (!sentry) {
        return make_result(0, sentry.error());
    }
    if (!s.chain().output_empty()) {
        auto r = s.chain().put(data);
        if (r.value() != 1 || r.has_error()) {
            return r;
        }
    }
    return s.device().put(data);
}

template <typename Stream>
typename Stream::formatter_type get_formatter(Stream& s)
{
    return s.formatter();
}

template <typename Stream>
result flush(Stream& s)
{
    auto sentry = typename Stream::output_sentry(s);
    if (!sentry) {
        return make_result(0, sentry.error());
    }
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
    auto sentry = typename Stream::input_sentry(s);
    if (!sentry) {
        return make_result(0, sentry.error());
    }
    bool eof = false;
    auto r = s.source().read(data, eof);
    if (r.has_error()) {
        putback(s, data.first(r.value()));
        return make_result(0, r.error());
    }
    if (eof) {
        s.set_eof();
    }
    if (!s.chain().input_empty()) {
        data = data.first(r.value());
        r = s.chain().read(data);
        if (r.has_error()) {
            putback(s, data);
            return make_result(0, r.error());
        }
        if (r.value() < data.size()) {
            putback(s, data.subspan(r.value()));
        }
    }
    return r;
}

template <typename Stream>
result read_at(Stream& s, gsl::span<gsl::byte> data, streampos pos)
{
    auto sentry = typename Stream::input_sentry(s);
    if (!sentry) {
        return make_result(0, sentry.error());
    }
    bool eof = false;
    auto r =
        s.device().read_at(data, Stream::character_type::to_device(pos), eof);
    if (r.has_error()) {
        putback(s, data.first(r.value()));
        return make_result(0, r.error());
    }
    if (eof) {
        s.set_eof();
    }
    if (!s.chain().input_empty()) {
        data = data.first(r.value());
        r = s.chain().read(data);
        if (r.has_error()) {
            putback(s, data);
            return make_result(0, r.error());
        }
        if (r.value() < data.size()) {
            putback(s, data.subspan(r.value()));
        }
    }
    return r;
}

template <typename Stream>
result get(Stream& s, gsl::byte& data)
{
    auto sentry = typename Stream::input_sentry(s);
    if (!sentry) {
        return make_result(0, sentry.error());
    }
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
    if (!s.chain().input_empty()) {
        r = s.chain().get(data);
        if (r.has_error()) {
            if (r.value() == 1) {
                putback(s, data);
            }
            return make_result(0, r.error());
        }
    }
    return r;
}

template <typename Stream>
auto putback(Stream& s, gsl::span<const gsl::byte> d) ->
    typename std::enable_if<is_readable<Stream>::value, bool>::type
{
    s.clear_eof();
    auto sentry = typename Stream::input_sentry(s);
    if (!sentry) {
        s.set_bad();
        return false;
    }
    return s.source().putback(d);
}
template <typename Stream>
auto putback(Stream& s, gsl::byte d) ->
    typename std::enable_if<is_byte_readable<Stream>::value &&
                                !is_readable<Stream>::value,
                            bool>::type
{
    s.clear_eof();
    auto sentry = typename Stream::input_sentry(s);
    if (!sentry) {
        s.set_bad();
        return false;
    }
    return s.device().putback(d);
}

template <typename Stream>
nonstd::expected<streampos, failure> seek(Stream& s,
                                          streampos pos,
                                          inout which = in | out)
{
    auto ret = s.device().seek(Stream::character_type::to_device(pos), which);
    if (ret) {
        ret.value() = Stream::character_type::from_device(ret.value());
    }
    return ret;
}
template <typename Stream>
nonstd::expected<streampos, failure> seek(Stream& s,
                                          streamoff off,
                                          seekdir dir,
                                          inout which = in | out)
{
    auto ret =
        s.device().seek(Stream::character_type::to_device(off), dir, which);
    if (ret) {
        ret.value() = Stream::character_type::from_device(ret.value());
    }
    return ret;
}
template <typename Stream>
nonstd::expected<streampos, failure> tell(Stream& s, inout which = in | out)
{
    return seek(s, 0, seekdir::cur, which);
}

SPIO_END_NAMESPACE
}  // namespace spio

#endif  // SPIO_STREAM_H
