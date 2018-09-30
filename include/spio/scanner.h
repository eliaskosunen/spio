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

// The contents of this file are heavily influenced by fmtlib:
//     https://github.com/fmtlib/fmt
//     https://fmtlib.net
//     https://wg21.link/p6045
// fmtlib is licensed under the BSD 2-clause license.
// Copyright (c) 2012-2018 Victor Zverovich

#ifndef SPIO_SCANNER_H
#define SPIO_SCANNER_H

#include "config.h"

#include "stream_ref.h"
#include "string_view.h"

namespace spio {
SPIO_BEGIN_NAMESPACE

template <typename CharT, typename T, typename Enable = void>
struct basic_scanner_impl;

template <typename CharT>
struct basic_scan_locale {
    span<const CharT> space;
    span<const CharT> thousand_sep;
    span<const CharT> decimal_sep;
    span<const CharT> true_str;
    span<const CharT> false_str;
};

template <typename CharT, size_t N>
span<CharT> strspan(CharT (&str)[N])
{
    return make_span(str, N - 1);
}

template <typename CharT>
basic_scan_locale<CharT> classic_scan_locale()
{
    SPIO_UNIMPLEMENTED;
}
template <>
inline basic_scan_locale<char> classic_scan_locale()
{
    static basic_scan_locale<char> locale{strspan(" \r\n\t\v"), strspan(" ,"),
                                          strspan("."), strspan("true"),
                                          strspan("false")};
    return locale;
}
template <>
inline basic_scan_locale<wchar_t> classic_scan_locale()
{
    static basic_scan_locale<wchar_t> locale{
        strspan(L" \r\n\t\v"), strspan(L" ,"), strspan(L"."), strspan(L"true"),
        strspan(L"false")};
    return locale;
}
template <>
inline basic_scan_locale<char16_t> classic_scan_locale()
{
    static basic_scan_locale<char16_t> locale{
        strspan(u" \r\n\t\v"), strspan(u" ,"), strspan(u"."), strspan(u"true"),
        strspan(u"false")};
    return locale;
}
template <>
inline basic_scan_locale<char32_t> classic_scan_locale()
{
    static basic_scan_locale<char32_t> locale{
        strspan(U" \r\n\t\v"), strspan(U" ,"), strspan(U"."), strspan(U"true"),
        strspan(U"false")};
    return locale;
}

namespace detail {
    template <typename Context>
    struct custom_value {
        using fn_type = expected<void, failure>(void*, Context&);

        void* value;
        fn_type* scan;
    };
}  // namespace detail

template <typename Context>
expected<void, failure> parse_whitespace(Context& ctx)
{
    bool found = false;
    auto in_span = [](typename Context::char_type ch,
                      span<const typename Context::char_type> s) {
        return std::find(s.begin(), s.end(), ch) != s.end();
    };
    while (in_span(*ctx.parse_context().begin(), ctx.locale().space)) {
        if (!found) {
            auto ch = ctx.stream().read_char();
            if (!ch) {
                return make_unexpected(ch.error());
            }
            if (in_span(ch.value(), ctx.locale().space)) {
                ctx.stream().putback(ch.value());
                found = true;
            }
        }
        ctx.parse_context().advance();
    }
    return {};
}

template <typename Context>
class basic_scan_arg {
public:
    using char_type = typename Context::char_type;

    template <typename T>
    explicit basic_scan_arg(T& val)
        : m_value{std::addressof(val), scan_custom_arg<T>}
    {
    }

    expected<void, failure> visit(Context& ctx)
    {
        return m_value.scan(m_value.value, ctx);
    }

private:
    template <typename T>
    static expected<void, failure> scan_custom_arg(void* arg, Context& ctx)
    {
        typename Context::template scanner_impl_type<T> s;
        auto err = s.parse(ctx);
        if (!err) {
            return err;
        }
        err = parse_whitespace(ctx);
        if (!err) {
            return err;
        }
        return s.scan(*static_cast<T*>(arg), ctx);
    }

    detail::custom_value<Context> m_value;
};

namespace detail {
    enum { max_packed_args = 15 };
}

template <typename Context>
class basic_scan_args;

template <typename Context, typename... Args>
class scan_arg_store {
public:
    scan_arg_store(Args&... a) : m_data{basic_scan_arg<Context>(a)...} {}

    span<basic_scan_arg<Context>> data()
    {
        return make_span(m_data.data(),
                         static_cast<std::ptrdiff_t>(m_data.size()));
    }

private:
    static SPIO_CONSTEXPR const size_t size = sizeof...(Args);

    std::array<basic_scan_arg<Context>, size> m_data;
};

template <typename Context, typename... Args>
scan_arg_store<Context, Args...> make_scan_args(Args&... args)
{
    return scan_arg_store<Context, Args...>(args...);
}

template <typename Context>
class basic_scan_args {
public:
    basic_scan_args() = default;

    template <typename... Args>
    basic_scan_args(scan_arg_store<Context, Args...>& store)
        : m_args(store.data())
    {
    }

    basic_scan_args(span<basic_scan_arg<Context>> args) : m_args(args) {}

    expected<void, failure> visit(Context& ctx)
    {
        for (auto& a : m_args) {
            auto ret = a.visit(ctx);
            if (!ret) {
                ctx.stream().putback_all();
                return ret;
            }
            ctx.parse_context().advance();
        }
        return {};
    }

private:
    span<basic_scan_arg<Context>> m_args;
};

template <typename Char>
class basic_scan_parse_context {
public:
    using char_type = Char;
    using iterator = typename basic_string_view<char_type>::iterator;

    explicit SPIO_CONSTEXPR basic_scan_parse_context(
        basic_string_view<char_type> f)
        : m_str(f)
    {
    }

    SPIO_CONSTEXPR iterator begin() const noexcept
    {
        return m_str.begin();
    }
    SPIO_CONSTEXPR iterator end() const noexcept
    {
        return m_str.end();
    }

    SPIO_CONSTEXPR14 iterator advance()
    {
        m_str.remove_prefix(1);
        return begin();
    }
    SPIO_CONSTEXPR14 void advance_to(iterator it)
    {
        m_str.remove_prefix(static_cast<size_t>(std::distance(begin(), it)));
    }

private:
    basic_string_view<char_type> m_str;
};

template <typename Char, typename Tag>
class basic_scan_stream_ref;

template <typename Char>
class basic_scan_stream_ref<Char, readable_tag> {
public:
    using ref_type = basic_stream_ref<Char, readable_tag>;
    using char_type = typename ref_type::char_type;

    basic_scan_stream_ref(ref_type ref) : m_ref(ref) {}

    expected<char_type, failure> read_char()
    {
        // TODO: encoding
        char_type ch{};
        auto ret = getchar(m_ref, ch);
        if (ret.has_error()) {
            return make_unexpected(ret.error());
        }
        m_buf.push_back(ch);
        return ch;
    }
    bool putback(char_type ch)
    {
        m_buf.pop_back();
        return putback(as_bytes(make_span(&ch, 1)));
    }

    bool putback_all()
    {
        auto ret = putback(as_bytes(make_span(m_buf.data(), m_buf.size())));
        if (ret) {
            m_buf.clear();
        }
        return ret;
    }

private:
    ref_type m_ref;
    std::vector<char_type> m_buf;
};
template <typename Char>
class basic_scan_stream_ref<Char, byte_readable_tag> {
public:
    using ref_type = basic_stream_ref<Char, byte_readable_tag>;
    using char_type = typename ref_type::char_type;

    basic_scan_stream_ref(ref_type ref) : m_ref(ref) {}

    expected<char_type, failure> read_char()
    {
        // TODO: encoding
        char_type ch{};
        auto ret = getchar(m_ref, ch);
        if (ret.has_error()) {
            return make_unexpected(ret.error());
        }
        m_buf.push_back(ch);
        return ch;
    }
    bool putback(char_type ch)
    {
        for (auto b : as_bytes(make_span(&ch, 1))) {
            if (!putback(b)) {
                return false;
            }
        }
        m_buf.pop_back();
        return true;
    }

    bool putback_all()
    {
        // FIXME
        for (auto b : as_bytes(make_span(m_buf.data(), 1))) {
            if (!putback(b)) {
                return false;
            }
        }
        m_buf.clear();
        return true;
    }

private:
    ref_type m_ref;
    std::vector<char_type> m_buf;
};
template <typename Char>
class basic_scan_stream_ref<Char, random_access_readable_tag> {
public:
    using ref_type = basic_stream_ref<Char, random_access_readable_tag>;
    using char_type = typename ref_type::char_type;

    basic_scan_stream_ref(ref_type ref, streampos pos) : m_ref(ref), m_pos(pos)
    {
    }

    expected<char_type, failure> read_char()
    {
        // TODO: encoding
        char_type ch{};
        auto ret = getchar_at(m_ref, ch, m_pos);
        if (ret.has_error()) {
            return make_unexpected(ret.error());
        }
        m_pos += ret.value();
        m_read += ret.value();
        return ch;
    }
    bool putback(char_type)
    {
        m_pos -= 1;
        m_read--;
        return true;
    }
    bool putback_all()
    {
        m_pos -= m_read;
        m_read = 0;
        return true;
    }

private:
    ref_type m_ref;
    streampos m_pos;
    streamoff m_read{0};
};

template <typename StreamRef, typename Encoding>
class basic_scan_context {
public:
    using ref_type = StreamRef;
    using encoding_type = Encoding;
    using char_type = typename Encoding::value_type;
    using parse_context_type = basic_scan_parse_context<char_type>;
    using locale_type = basic_scan_locale<char_type>;

    template <typename T>
    using scanner_impl_type = basic_scanner_impl<char_type, T>;

    basic_scan_context(ref_type r,
                       basic_string_view<char_type> f,
                       basic_scan_locale<char_type> locale)
        : m_ref(std::move(r)), m_parse_ctx(f), m_locale(locale)
    {
    }

    parse_context_type& parse_context()
    {
        return m_parse_ctx;
    }

    ref_type& stream()
    {
        return m_ref;
    }

    const locale_type& locale() const
    {
        return m_locale;
    }

private:
    ref_type m_ref;
    parse_context_type m_parse_ctx;
    locale_type m_locale;
};

namespace detail {
    template <typename CharT>
    struct scanner_parser_empty {
        template <typename Context>
        expected<void, failure> parse(Context& ctx)
        {
            if (*ctx.parse_context().begin() != CharT('{')) {
                return make_unexpected(failure{
                    scanner_error,
                    fmt::format("Unexpected '{}' in scanner format string",
                                ctx.parse_context().begin())});
            }
            ctx.parse_context().advance();
            return {};
        }
    };
}  // namespace detail

template <typename CharT>
struct basic_scanner_impl<CharT, CharT>
    : public detail::scanner_parser_empty<CharT> {
    template <typename Context>
    expected<void, failure> scan(CharT& val, Context& ctx)
    {
        auto ch = ctx.stream().read_char();
        if (!ch) {
            return make_unexpected(ch.error());
        }
        val = ch.value();
        return {};
    }
};
template <typename CharT>
struct basic_scanner_impl<CharT, span<CharT>>
    : public detail::scanner_parser_empty<CharT> {
    template <typename Context>
    expected<void, failure> scan(span<CharT> val, Context& ctx)
    {
        if (val.size() == 0) {
            return {};
        }

        auto in_span = [](CharT ch, span<const CharT> s) {
            return std::find(s.begin(), s.end(), ch) != s.end();
        };
        std::vector<CharT> buf(static_cast<size_t>(val.size()));
        auto it = buf.begin();
        for (; it != buf.end(); ++it) {
            auto ch = ctx.stream().read_char();
            if (!ch) {
                for (auto i = buf.begin(); i != it - 1; ++i) {
                    ctx.stream().putback(*i);
                }
                return make_unexpected(ch.error());
            }
            if (in_span(ch.value(), ctx.locale().space)) {
                break;
            }
            *it = ch.value();
        }
        std::copy(buf.begin(), buf.end(), val.begin());

        return {};
    }
};
template <typename CharT>
struct basic_scanner_impl<CharT, bool>
    : public detail::scanner_parser_empty<CharT> {
    template <typename Context>
    expected<void, failure> scan(bool& val, Context& ctx)
    {
        auto tmp = ctx.stream().read_char();
        if (!tmp) {
            return make_unexpected(tmp.error());
        }
        if (tmp == CharT('0')) {
            val = false;
            return {};
        }
        if (tmp == CharT('1')) {
            val = true;
            return {};
        }
        ctx.stream().putback(tmp.value());

        const auto max_len = std::max(ctx.locale().true_str.size(),
                                      ctx.locale().false_str.size());
        if (max_len < 1) {
            return make_unexpected(
                failure{scanner_error, "Invalid boolean value"});
        }
        std::vector<CharT> buf(static_cast<size_t>(max_len));
        auto it = buf.begin();
        for (; it != buf.end(); ++it) {
            auto ch = ctx.stream().read_char();
            if (!ch) {
                for (auto i = buf.begin(); i != it - 1; ++i) {
                    ctx.stream().putback(*i);
                }
                return make_unexpected(ch.error());
            }
            *it = ch.value();

            if (std::equal(buf.begin(), it + 1,
                           ctx.locale().false_str.begin())) {
                val = false;
                return {};
            }
            if (std::equal(buf.begin(), it + 1,
                           ctx.locale().true_str.begin())) {
                val = true;
                return {};
            }
        }

        return make_unexpected(failure{scanner_error, "Invalid boolean value"});
    }
};
template <typename CharT, typename T>
struct basic_scanner_impl<
    CharT,
    T,
    typename std::enable_if<std::is_integral<T>::value &&
                            !std::is_same<T, CharT>::value &&
                            !std::is_same<T, bool>::value>::type> {
    template <typename Context>
    expected<void, failure> parse(Context& ctx)
    {
        ctx.parse_context().advance();
        const auto ch = *ctx.parse_context().begin();
        if (ch == CharT('d') || ch == CharT('}')) {
            base = 10;
        }
        else if (ch == CharT('x')) {
            base = 16;
        }
        else if (ch == CharT('o')) {
            base = 8;
        }
        else if (ch == CharT('b')) {
            base = 2;
        }
        else {
            return make_unexpected(
                failure{scanner_error,
                        "Integral types can only be scanned in bases 10 (d), "
                        "16 (x), 8 (o) and 2 (b)"});
        }
        if (ch != CharT('}')) {
            ctx.parse_context().advance();
        }
        return {};
    }

    template <typename Context>
    expected<void, failure> scan(T& val, Context& ctx)
    {
        std::vector<CharT> buf(static_cast<size_t>(max_digits<T>()) + 1);

        // Copied from span<CharT>
        auto in_span = [](CharT ch, span<const CharT> s) {
            return std::find(s.begin(), s.end(), ch) != s.end();
        };
        for (auto it = buf.begin(); it != buf.end(); ++it) {
            auto ch = ctx.stream().read_char();
            if (!ch) {
                for (auto i = buf.begin(); i != it - 1; ++i) {
                    ctx.stream().putback(*i);
                }
                return make_unexpected(ch.error());
            }
            if (in_span(ch.value(), ctx.locale().space)) {
                break;
            }
            *it = ch.value();
        }

        T tmp = 0;
        auto it = buf.begin();
        auto sign_tmp = [&]() -> expected<bool, failure> {
            if (std::is_unsigned<T>::value) {
                if (*it == CharT('-')) {
                    return make_unexpected(failure{
                        scanner_error,
                        "Cannot scan a signed integer into an unsigned value"});
                }
            }
            else {
                if (*it == CharT('-')) {
                    return false;
                }
            }

            if (*it == CharT('+')) {
                return true;
            }
            if (is_digit(*it, base)) {
                tmp = tmp * static_cast<T>(base) - char_to_int<T>(*it, base);
                return true;
            }
            return make_unexpected(failure{
                scanner_error, "Invalid first character in scanned integer"});
        }();
        if (!sign_tmp) {
            return make_unexpected(sign_tmp.error());
        }
        const bool sign = sign_tmp.value();
        ++it;

        for (; it != buf.end(); ++it) {
            if (is_digit(*it, base)) {
                tmp = tmp * static_cast<T>(base) - char_to_int<T>(*it, base);
            }
            else {
                break;
            }
        }

        if (sign) {
#if SPIO_MSVC
#pragma warning(push)
#pragma warning(disable : 4146)
#endif
            tmp = -tmp;
#if SPIO_MSVC
#pragma warning(pop)
#endif
        }

        val = tmp;
        return {};
    }

    int base{10};
};
template <typename CharT, typename T>
struct basic_scanner_impl<
    CharT,
    T,
    typename std::enable_if<std::is_floating_point<T>::value>::type>
    : public detail::scanner_parser_empty<CharT> {
    template <typename Context>
    expected<void, failure> scan(T& val, Context& ctx)
    {
        std::array<CharT, 64> buf{};
        buf.fill(CharT{0});

        bool point = false;
        for (auto& c : buf) {

        }
    }
};

template <typename CharT>
struct basic_scanner {
    template <typename Context>
    expected<void, failure> operator()(Context& ctx,
                                       basic_scan_args<Context> args)
    {
        return args.visit(ctx);
    }
};

template <typename Stream, typename... Args>
auto scan(Stream& s,
          basic_string_view<typename Stream::char_type> f,
          Args&... a)
    -> decltype(read(std::declval<Stream&>(), std::declval<span<byte>>()),
                expected<void, failure>())
{
    using encoding_type = typename Stream::encoding_type;
    using ref_type = basic_scan_stream_ref<encoding_type, readable_tag>;
    using context_type = basic_scan_context<ref_type, encoding_type>;
    using args_type = basic_scan_args<context_type>;

    auto r = typename ref_type::ref_type(s);
    auto ref = ref_type(r);
    auto ctx =
        context_type(ref, f, classic_scan_locale<typename Stream::char_type>());
    auto args = make_scan_args<context_type>(a...);
    return get_scanner(ref)(ctx, args_type(args.data()));
}
template <typename Stream, typename... Args>
auto scan(Stream& s,
          basic_string_view<typename Stream::char_type> f,
          Args&... a)
    -> decltype(get(std::declval<Stream&>(), std::declval<byte>()),
                expected<void, failure>())
{
    using encoding_type = typename Stream::encoding_type;
    using ref_type = basic_scan_stream_ref<encoding_type, byte_readable_tag>;
    using context_type = basic_scan_context<ref_type, encoding_type>;
    using args_type = basic_scan_args<context_type>;

    auto r = typename ref_type::ref_type(s);
    auto ref = ref_type(r);
    auto ctx =
        context_type(ref, f, classic_scan_locale<typename Stream::char_type>());
    auto args = make_scan_args<context_type>(a...);
    return get_scanner(r)(ctx, args_type(args.data()));
}
template <typename Stream, typename... Args>
auto scan_at(Stream& s,
             streampos pos,
             basic_string_view<typename Stream::char_type> f,
             Args&... a) -> expected<void, failure>
{
    using encoding_type = typename Stream::encoding_type;
    using ref_type =
        basic_scan_stream_ref<encoding_type, random_access_readable_tag>;
    using context_type = basic_scan_context<ref_type, encoding_type>;
    using args_type = basic_scan_args<context_type>;

    auto r = typename ref_type::ref_type(s);
    auto ref = ref_type(r, pos);
    auto ctx =
        context_type(ref, f, classic_scan_locale<typename Stream::char_type>());
    auto args = make_scan_args<context_type>(a...);
    return get_scanner(r)(ctx, args_type(args.data()));
}

template <typename Char,
          typename Tag = make_tag<readable_tag, putbackable_span_tag>,
          typename... Args>
auto scan(basic_stream_ref<Char, Tag> s,
          basic_string_view<typename Char::type> f,
          Args&... a) -> expected<void, failure>
{
    using ref_type = basic_scan_stream_ref<Char, readable_tag>;
    using context_type = basic_scan_context<ref_type, Char>;
    using args_type = basic_scan_args<context_type>;

    auto ref = ref_type(s);
    auto ctx = context_type(ref, f, classic_scan_locale<typename Char::type>());
    auto args = make_scan_args<context_type>(a...);
    return get_scanner(s)(ctx, args_type(args.data()));
}
template <typename Char,
          typename Tag = random_access_readable_tag,
          typename... Args>
auto scan_at(basic_stream_ref<Char, Tag> s,
             streampos pos,
             basic_string_view<typename Char::type> f,
             Args&... a) -> expected<void, failure>
{
    using ref_type = basic_scan_stream_ref<Char, random_access_readable_tag>;
    using context_type = basic_scan_context<ref_type, Char>;
    using args_type = basic_scan_args<context_type>;

    auto ref = ref_type(s, pos);
    auto ctx = context_type(ref, f, classic_scan_locale<typename Char::type>());
    auto args = make_scan_args<context_type>(a...);
    return get_scanner(s)(ctx, args_type(args.data()));
}

namespace detail {
    template <typename Stream>
    template <typename S>
    auto erased_stream_storage<Stream>::_scanner() ->
        typename std::enable_if<is_source<typename S::device_type>::value,
                                basic_scanner<typename S::encoding_type>>::type
    {
        return m_stream.scanner();
    }

    template <typename Stream>
    template <typename S>
    [[noreturn]] auto erased_stream_storage<Stream>::_scanner() ->
        typename std::enable_if<!is_source<typename S::device_type>::value,
                                basic_scanner<typename S::encoding_type>>::type
    {
        SPIO_UNREACHABLE;
    }
}  // namespace detail

SPIO_END_NAMESPACE
}  // namespace spio

#endif  // SPIO_SCANNER_H
