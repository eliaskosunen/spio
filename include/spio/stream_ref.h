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

#ifndef SPIO_STREAM_REF_H
#define SPIO_STREAM_REF_H

#include "config.h"

#include "stream.h"

SPIO_BEGIN_NAMESPACE

namespace detail {
    template <typename Char>
    struct erased_stream_storage_base {
        virtual bool bad() const = 0;
        virtual bool eof() const = 0;
        virtual explicit operator bool() const = 0;
        bool operator!() const
        {
            return !operator bool();
        }

        virtual void set_bad() = 0;
        virtual void clear_bad() = 0;
        virtual void set_eof() = 0;
        virtual void clear_eof() = 0;

        virtual bool is_open() const = 0;
        virtual nonstd::expected<void, failure> close() = 0;

        virtual result write(std::vector<gsl::byte> buf) = 0;
        virtual result write(gsl::span<const gsl::byte> buf) = 0;
        virtual result write_at(std::vector<gsl::byte> buf, streampos pos) = 0;
        virtual result write_at(gsl::span<const gsl::byte> buf,
                                streampos pos) = 0;
        virtual result put(gsl::byte data) = 0;

        virtual result flush() = 0;
        virtual nonstd::expected<void, failure> sync() = 0;

        virtual basic_formatter<typename Char::type> formatter() = 0;

        virtual result read(gsl::span<gsl::byte> buf) = 0;
        virtual result read_at(gsl::span<gsl::byte> buf, streampos pos) = 0;
        virtual result get(gsl::byte& data) = 0;

        virtual bool putback(gsl::span<const gsl::byte> data) = 0;
        virtual bool putback(gsl::byte data) = 0;

        virtual nonstd::expected<streampos, failure> seek(
            streampos pos,
            inout which = in | out) = 0;
        virtual nonstd::expected<streampos, failure>
        seek(streamoff off, seekdir dir, inout which = in | out) = 0;
        virtual nonstd::expected<streampos, failure> tell(
            inout which = in | out) = 0;

        virtual ~erased_stream_storage_base() = default;
    };
    template <typename Stream>
    class erased_stream_storage
        : public erased_stream_storage_base<typename Stream::character_type> {
    public:
        using stream_type = Stream;

        erased_stream_storage(Stream& s) : m_stream(s) {}

        Stream& get_stream()
        {
            return m_stream;
        }
        const Stream& get_stream() const
        {
            return m_stream;
        }

        bool bad() const override
        {
            return m_stream.bad();
        }
        bool eof() const override
        {
            return m_stream.eof();
        }
        explicit operator bool() const override
        {
            return m_stream.operator bool();
        }

        void set_bad() override
        {
            m_stream.set_bad();
        }
        void clear_bad() override
        {
            m_stream.clear_bad();
        }
        void set_eof() override
        {
            m_stream.set_eof();
        }
        void clear_eof() override
        {
            m_stream.clear_eof();
        }

        bool is_open() const override
        {
            return m_stream.is_open();
        }
        nonstd::expected<void, failure> close() override
        {
            return m_stream.close();
        }

        result write(std::vector<gsl::byte> buf) override
        {
            return _write(buf);
        }
        result write(gsl::span<const gsl::byte> buf) override
        {
            return _write(buf);
        }
        result write_at(std::vector<gsl::byte> buf, streampos pos) override
        {
            return _write_at(buf, pos);
        }
        result write_at(gsl::span<const gsl::byte> buf, streampos pos) override
        {
            return _write_at(buf, pos);
        }
        result put(gsl::byte data) override
        {
            return _put(data);
        }

        result flush() override
        {
            return _flush();
        }
        nonstd::expected<void, failure> sync() override
        {
            return _sync();
        }

        basic_formatter<typename Stream::char_type> formatter() override
        {
            return _formatter();
        }

        result read(gsl::span<gsl::byte> buf) override
        {
            return _read(buf);
        }
        result read_at(gsl::span<gsl::byte> buf, streampos pos) override
        {
            return _read_at(buf, pos);
        }
        result get(gsl::byte& data) override
        {
            return _get(data);
        }

        bool putback(gsl::span<const gsl::byte> buf) override
        {
            return _putback(buf);
        }
        bool putback(gsl::byte d) override
        {
            return _putback(d);
        }

        nonstd::expected<streampos, failure> seek(streampos pos,
                                                  inout which = in |
                                                                out) override
        {
            return _seek(pos, which);
        }
        nonstd::expected<streampos, failure> seek(streamoff off,
                                                  seekdir dir,
                                                  inout which = in |
                                                                out) override
        {
            return _seek(off, dir, which);
        }
        nonstd::expected<streampos, failure> tell(inout which = in |
                                                                out) override
        {
            return _tell(which);
        }

    private:
        template <typename S = Stream>
        auto _write(std::vector<gsl::byte> buf) ->
            typename std::enable_if<is_writable_stream<S>::value, result>::type
        {
            return ::spio::write(m_stream, buf);
        }
        template <typename S = Stream>
        [[noreturn]] auto _write(std::vector<gsl::byte>) ->
            typename std::enable_if<!is_writable_stream<S>::value, result>::type
        {
            SPIO_UNREACHABLE;
        }
        template <typename S = Stream>
        auto _write(gsl::span<const gsl::byte> buf) ->
            typename std::enable_if<is_writable_stream<S>::value, result>::type
        {
            return ::spio::write(m_stream, buf);
        }
        template <typename S = Stream>
        [[noreturn]] auto _write(gsl::span<const gsl::byte>) ->
            typename std::enable_if<!is_writable_stream<S>::value, result>::type
        {
            SPIO_UNREACHABLE;
        }

        template <typename S = Stream>
        auto _write_at(std::vector<gsl::byte> buf, streampos pos) ->
            typename std::enable_if<is_random_access_writable_stream<S>::value,
                                    result>::type
        {
            return ::spio::write_at(m_stream, buf, pos);
        }
        template <typename S = Stream>
        [[noreturn]] auto _write_at(std::vector<gsl::byte>, streampos) ->
            typename std::enable_if<!is_random_access_writable_stream<S>::value,
                                    result>::type
        {
            SPIO_UNREACHABLE;
        }
        template <typename S = Stream>
        auto _write_at(gsl::span<const gsl::byte> buf, streampos pos) ->
            typename std::enable_if<is_random_access_writable_stream<S>::value,
                                    result>::type
        {
            return ::spio::write_at(m_stream, buf, pos);
        }
        template <typename S = Stream>
        [[noreturn]] auto _write_at(gsl::span<const gsl::byte>, streampos) ->
            typename std::enable_if<!is_random_access_writable_stream<S>::value,
                                    result>::type
        {
            SPIO_UNREACHABLE;
        }

        template <typename S = Stream>
        auto _put(gsl::byte data) ->
            typename std::enable_if<is_byte_writable_stream<S>::value,
                                    result>::type
        {
            return ::spio::put(m_stream, data);
        }
        template <typename S = Stream>
        [[noreturn]] auto _put(gsl::byte) ->
            typename std::enable_if<!is_byte_writable_stream<S>::value,
                                    result>::type
        {
            SPIO_UNREACHABLE;
        }

        template <typename S = Stream>
        auto _flush() ->
            typename std::enable_if<is_flushable_stream<S>::value, result>::type
        {
            return ::spio::flush(m_stream);
        }
        template <typename S = Stream>
        [[noreturn]] auto _flush() ->
            typename std::enable_if<!is_flushable_stream<S>::value,
                                    result>::type
        {
            SPIO_UNREACHABLE;
        }

        template <typename S = Stream>
        auto _sync() ->
            typename std::enable_if<is_syncable_stream<S>::value,
                                    nonstd::expected<void, failure>>::type
        {
            return ::spio::sync(m_stream);
        }
        template <typename S = Stream>
        [[noreturn]] auto _sync() ->
            typename std::enable_if<!is_syncable_stream<S>::value,
                                    nonstd::expected<void, failure>>::type
        {
            SPIO_UNREACHABLE;
        }

        template <typename S = Stream>
        auto _formatter() -> typename std::enable_if<
            is_sink<typename S::device_type>::value,
            basic_formatter<typename S::char_type>>::type
        {
            return m_stream.formatter();
        }
        template <typename S = Stream>
        [[noreturn]] auto _formatter() -> typename std::enable_if<
            !is_sink<typename S::device_type>::value,
            basic_formatter<typename S::char_type>>::type
        {
            SPIO_UNREACHABLE;
        }

        template <typename S = Stream>
        auto _read(gsl::span<gsl::byte> buf) ->
            typename std::enable_if<is_readable_stream<S>::value, result>::type
        {
            return ::spio::read(m_stream, buf);
        }
        template <typename S = Stream>
        [[noreturn]] auto _read(gsl::span<gsl::byte>) ->
            typename std::enable_if<!is_readable_stream<S>::value, result>::type
        {
            SPIO_UNREACHABLE;
        }

        template <typename S = Stream>
        auto _read_at(gsl::span<gsl::byte> buf, streampos pos) ->
            typename std::enable_if<is_random_access_readable_stream<S>::value,
                                    result>::type
        {
            return ::spio::read_at(m_stream, buf, pos);
        }
        template <typename S = Stream>
        [[noreturn]] auto _read_at(gsl::span<gsl::byte>, streampos) ->
            typename std::enable_if<!is_random_access_readable_stream<S>::value,
                                    result>::type
        {
            SPIO_UNREACHABLE;
        }

        template <typename S = Stream>
        auto _get(gsl::byte& d) ->
            typename std::enable_if<is_byte_readable_stream<S>::value,
                                    result>::type
        {
            return ::spio::get(m_stream, d);
        }
        template <typename S = Stream>
        [[noreturn]] auto _get(gsl::byte&) ->
            typename std::enable_if<!is_byte_readable_stream<S>::value,
                                    result>::type
        {
            SPIO_UNREACHABLE;
        }

        template <typename S = Stream>
        auto _putback(gsl::span<const gsl::byte> buf) ->
            typename std::enable_if<is_putbackable_span_stream<S>::value,
                                    bool>::type
        {
            return ::spio::putback(m_stream, buf);
        }
        template <typename S = Stream>
        [[noreturn]] auto _putback(gsl::span<const gsl::byte>) ->
            typename std::enable_if<!is_putbackable_span_stream<S>::value,
                                    bool>::type
        {
            SPIO_UNREACHABLE;
        }

        template <typename S = Stream>
        auto _putback(gsl::byte d) ->
            typename std::enable_if<is_putbackable_byte_stream<S>::value,
                                    bool>::type
        {
            return ::spio::putback(m_stream, d);
        }
        template <typename S = Stream>
        [[noreturn]] auto _putback(gsl::byte) ->
            typename std::enable_if<!is_putbackable_byte_stream<S>::value,
                                    bool>::type
        {
            SPIO_UNREACHABLE;
        }

        template <typename S = Stream>
        auto _seek(streampos pos, inout which = in | out) ->
            typename std::enable_if<is_absolute_seekable_stream<S>::value,
                                    nonstd::expected<streampos, failure>>::type
        {
            return ::spio::seek(m_stream, pos, which);
        }
        template <typename S = Stream>
        auto _seek(streampos, inout = in | out) ->
            typename std::enable_if<!is_absolute_seekable_stream<S>::value,
                                    nonstd::expected<streampos, failure>>::type
        {
            SPIO_UNREACHABLE;
        }

        template <typename S = Stream>
        auto _seek(streamoff off, seekdir dir, inout which = in | out) ->
            typename std::enable_if<is_relative_seekable_stream<S>::value,
                                    nonstd::expected<streampos, failure>>::type
        {
            return ::spio::seek(m_stream, off, dir, which);
        }
        template <typename S = Stream>
        auto _seek(streamoff, seekdir, inout = in | out) ->
            typename std::enable_if<!is_relative_seekable_stream<S>::value,
                                    nonstd::expected<streampos, failure>>::type
        {
            SPIO_UNREACHABLE;
        }

        template <typename S = Stream>
        auto _tell(inout which = in | out) ->
            typename std::enable_if<is_tellable_stream<S>::value,
                                    nonstd::expected<streampos, failure>>::type
        {
            return ::spio::tell(m_stream, which);
        }
        template <typename S = Stream>
        auto _tell(inout = in | out) ->
            typename std::enable_if<!is_tellable_stream<S>::value,
                                    nonstd::expected<streampos, failure>>::type
        {
            SPIO_UNREACHABLE;
        }

        Stream& m_stream;
    };
    template <typename Char>
    class basic_erased_stream {
        template <typename Device, template <typename...> class Chain>
        using stream_type = stream<Device, Char, Chain>;

    public:
        using base = erased_stream_storage_base<Char>;
        using pointer = std::unique_ptr<base>;

        basic_erased_stream() = default;
        basic_erased_stream(pointer p) : m_ptr(std::move(p)) {}

        template <
            typename Device,
            template <typename...> class Chain,
            typename T = erased_stream_storage<stream_type<Device, Chain>>>
        basic_erased_stream(stream_type<Device, Chain>& s)
            : m_ptr(make_unique<T>(s))
        {
        }

        template <
            typename Device,
            template <typename...> class Chain,
            typename T = erased_stream_storage<stream_type<Device, Chain>>>
        void set(stream_type<Device, Chain>& s)
        {
            m_ptr = make_unique<T>(s);
        }

        bool valid() const
        {
            return m_ptr.operator bool();
        }
        explicit operator bool() const
        {
            return valid();
        }

        base& get()
        {
            return *m_ptr;
        }
        const base& get() const
        {
            return *m_ptr;
        }

        base& operator*()
        {
            return get();
        }
        const base& operator*() const
        {
            return get();
        }

        base* operator->()
        {
            return m_ptr.get();
        }
        const base* operator->() const
        {
            return m_ptr.get();
        }

    private:
        pointer m_ptr;
    };
}  // namespace detail

struct any_tag {
};

struct sink_tag : virtual any_tag {
};
struct writable_tag : sink_tag {
};
struct random_access_writable_tag : sink_tag {
};
struct byte_writable_tag : sink_tag {
};

struct flushable_tag : virtual any_tag {
};
struct syncable_tag : virtual any_tag {
};

struct source_tag : virtual any_tag {
};
struct readable_tag : source_tag {
};
struct random_access_readable_tag : source_tag {
};
struct byte_readable_tag : source_tag {
};

struct putbackable_span_tag : virtual any_tag {
};
struct putbackable_byte_tag : virtual any_tag {
};

struct absolute_seekable_tag : virtual any_tag {
};
struct relative_seekable_tag : virtual any_tag {
};
struct seekable_tag : absolute_seekable_tag, relative_seekable_tag {
};
struct tellable_tag : virtual any_tag {
};

namespace detail {
    template <typename T, typename Tag>
    using has_tag = disjunction<std::is_same<Tag, T>, std::is_base_of<Tag, T>>;
    template <typename Tag, typename T, typename U>
    using allowed_tag_conversion =
        std::integral_constant<bool,
                               !(!has_tag<T, Tag>::value &&
                                 has_tag<U, Tag>::value)>;
    template <typename T, typename U>
    using allowed_properties_conversion = std::integral_constant<
        bool,
        allowed_tag_conversion<writable_tag, T, U>::value() &&
            allowed_tag_conversion<random_access_writable_tag, T, U>::value() &&
            allowed_tag_conversion<byte_writable_tag, T, U>::value() &&

            allowed_tag_conversion<flushable_tag, T, U>::value() &&
            allowed_tag_conversion<syncable_tag, T, U>::value() &&

            allowed_tag_conversion<readable_tag, T, U>::value() &&
            allowed_tag_conversion<random_access_readable_tag, T, U>::value() &&
            allowed_tag_conversion<byte_readable_tag, T, U>::value() &&

            allowed_tag_conversion<putbackable_span_tag, T, U>::value() &&
            allowed_tag_conversion<putbackable_byte_tag, T, U>::value() &&

            allowed_tag_conversion<absolute_seekable_tag, T, U>::value() &&
            allowed_tag_conversion<relative_seekable_tag, T, U>::value() &&
            allowed_tag_conversion<tellable_tag, T, U>::value()>;

    template <typename Stream, typename Properties>
    using allowed_properties_construction = std::integral_constant<
        bool,
        (!has_tag<Properties, writable_tag>::value ||
         is_writable_stream<Stream>::value) &&
            (!has_tag<Properties, random_access_writable_tag>::value ||
             is_random_access_writable_stream<Stream>::value) &&
            (!has_tag<Properties, byte_writable_tag>::value ||
             is_byte_writable_stream<Stream>::value) &&

            (!has_tag<Properties, flushable_tag>::value ||
             is_flushable_stream<Stream>::value) &&
            (!has_tag<Properties, syncable_tag>::value ||
             is_syncable_stream<Stream>::value) &&

            (!has_tag<Properties, readable_tag>::value ||
             is_readable_stream<Stream>::value) &&
            (!has_tag<Properties, random_access_readable_tag>::value ||
             is_random_access_readable_stream<Stream>::value) &&
            (!has_tag<Properties, byte_readable_tag>::value ||
             is_byte_readable_stream<Stream>::value) &&

            (!has_tag<Properties, putbackable_span_tag>::value ||
             is_putbackable_span_stream<Stream>::value) &&
            (!has_tag<Properties, putbackable_byte_tag>::value ||
             is_putbackable_byte_stream<Stream>::value) &&

            (!has_tag<Properties, absolute_seekable_tag>::value ||
             is_absolute_seekable_stream<Stream>::value) &&
            (!has_tag<Properties, relative_seekable_tag>::value ||
             is_relative_seekable_stream<Stream>::value) &&
            (!has_tag<Properties, tellable_tag>::value ||
             is_tellable_stream<Stream>::value)>;
}  // namespace detail

template <typename Char, typename Properties>
class basic_stream_ref {
    using erased_stream = detail::basic_erased_stream<Char>;

public:
    using character_type = Char;
    using char_type = typename Char::type;

    basic_stream_ref() = default;
    template <typename Stream,
              typename std::enable_if<detail::allowed_properties_construction<
                  Stream,
                  Properties>::value>::type* = nullptr>
    basic_stream_ref(Stream& s) : m_stream(s)
    {
    }

    template <typename Stream>
    auto reset(Stream& s) -> typename std::enable_if<
        detail::allowed_properties_construction<Stream,
                                                Properties>::value>::type
    {
        m_stream.set(s);
    }

    template <typename P, typename Ret = basic_stream_ref<Char, P>>
    auto as() -> typename std::enable_if<
        detail::allowed_properties_conversion<Properties, P>::value,
        Ret>::type
    {
        return Ret(m_stream);
    }

    erased_stream& get() &
    {
        return m_stream;
    }
    const erased_stream& get() const&
    {
        return m_stream;
    }
    erased_stream&& get() &&
    {
        return m_stream;
    }

    erased_stream& operator*()
    {
        return m_stream;
    }
    const erased_stream& operator*() const
    {
        return m_stream;
    }

    typename erased_stream::base* operator->()
    {
        return m_stream.operator->();
    }
    const typename erased_stream::base* operator->() const
    {
        return m_stream.operator->();
    }

private:
    erased_stream m_stream;
};

template <typename Char, typename Properties>
auto write(basic_stream_ref<Char, Properties>& s, std::vector<gsl::byte> buf) ->
    typename std::enable_if<detail::has_tag<Properties, writable_tag>::value,
                            result>::type
{
    return s->write(std::move(buf));
}
template <typename Char, typename Properties>
auto write(basic_stream_ref<Char, Properties>& s,
           gsl::span<const gsl::byte> buf) ->
    typename std::enable_if<detail::has_tag<Properties, writable_tag>::value,
                            result>::type
{
    return s->write(buf);
}

template <typename Char, typename Properties>
auto write_at(basic_stream_ref<Char, Properties>& s,
              std::vector<gsl::byte> buf,
              streampos pos) ->
    typename std::enable_if<
        detail::has_tag<Properties, random_access_writable_tag>::value,
        result>::type
{
    return s->write_at(std::move(buf), pos);
}
template <typename Char, typename Properties>
auto write_at(basic_stream_ref<Char, Properties>& s,
              gsl::span<const gsl::byte> data,
              streampos pos) ->
    typename std::enable_if<
        detail::has_tag<Properties, random_access_writable_tag>::value,
        result>::type
{
    return s->write_at(data, pos);
}

template <typename Char, typename Properties>
auto put(basic_stream_ref<Char, Properties>& s, gsl::byte data) ->
    typename std::enable_if<
        detail::has_tag<Properties, byte_writable_tag>::value,
        result>::type
{
    return s->put(data);
}

template <typename Char, typename Properties>
auto flush(basic_stream_ref<Char, Properties>& s) ->
    typename std::enable_if<detail::has_tag<Properties, flushable_tag>::value,
                            result>::type
{
    return s->flush();
}
template <typename Char, typename Properties>
auto sync(basic_stream_ref<Char, Properties>& s) ->
    typename std::enable_if<detail::has_tag<Properties, syncable_tag>::value,
                            nonstd::expected<void, failure>>::type
{
    return s->sync();
}

template <typename Char, typename Properties>
auto read(basic_stream_ref<Char, Properties>& s, gsl::span<gsl::byte> data) ->
    typename std::enable_if<detail::has_tag<Properties, readable_tag>::value,
                            result>::type
{
    return s->read(data);
}

template <typename Char, typename Properties>
auto read_at(basic_stream_ref<Char, Properties>& s,
             gsl::span<gsl::byte> data,
             streampos pos) ->
    typename std::enable_if<
        detail::has_tag<Properties, random_access_readable_tag>::value,
        result>::type
{
    return s->read_at(data, pos);
}

template <typename Char, typename Properties>
auto get(basic_stream_ref<Char, Properties>& s, gsl::byte& data) ->
    typename std::enable_if<
        detail::has_tag<Properties, byte_readable_tag>::value,
        result>::type
{
    return s->get(data);
}

template <typename Char, typename Properties>
auto putback(basic_stream_ref<Char, Properties>& s,
             gsl::span<const gsl::byte> d) ->
    typename std::enable_if<
        detail::has_tag<Properties, putbackable_span_tag>::value,
        bool>::type
{
    return s->putback(d);
}
template <typename Char, typename Properties>
auto putback(basic_stream_ref<Char, Properties>& s, gsl::byte d) ->
    typename std::enable_if<
        detail::has_tag<Properties, putbackable_byte_tag>::value,
        bool>::type
{
    return s->putback(d);
}

template <typename Char, typename Properties>
auto seek(basic_stream_ref<Char, Properties>& s,
          streampos pos,
          inout which = in | out) ->
    typename std::enable_if<
        detail::has_tag<Properties, absolute_seekable_tag>::value,
        nonstd::expected<streampos, failure>>::type
{
    return s->seek(pos, which);
}
template <typename Char, typename Properties>
auto seek(basic_stream_ref<Char, Properties>& s,
          streamoff off,
          seekdir dir,
          inout which = in | out) ->
    typename std::enable_if<
        detail::has_tag<Properties, relative_seekable_tag>::value,
        nonstd::expected<streampos, failure>>::type
{
    return s->seek(off, dir, which);
}
template <typename Char, typename Properties>
auto tell(basic_stream_ref<Char, Properties>& s, inout which = in | out) ->
    typename std::enable_if<detail::has_tag<Properties, tellable_tag>::value,
                            nonstd::expected<streampos, failure>>::type
{
    return s->tell(which);
}

SPIO_END_NAMESPACE

#endif  // SPIO_STREAM_REF_H
